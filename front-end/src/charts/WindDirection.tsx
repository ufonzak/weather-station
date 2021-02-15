import React from 'react';
import _ from 'lodash';
import moment from 'moment';
import { Area, AreaChart, CartesianGrid, Legend, ResponsiveContainer, Tooltip, YAxis } from 'recharts';

import { environment } from '../environment';
import { Query } from '../query';
import { WindDirection, WIND_DIRECTIONS } from '../utils';
import { Range } from './utils';
import { timeAxis } from './TimeAxis';
import { ChartLoader } from './ChartLoader';

const measures = _.range(8).map(i => `wind_direction${i}_1h`);
const COLORS = [
  '#F51818',
  '#F50CF5',
  '#0C21F5',
  '#0AB8F5',
  '#18F557',
  '#D6F518',
  '#F5C518',
  '#F55A00',
];

interface Point extends Record<WindDirection, Number> {
  time: number;
}

interface Props {
  refreshKey?: number | string;
  range: Range;
}

export class WindDirectionChart extends React.Component<Props> {
  transformData(data: Query.Rows): Point[] { // TODO: memoize
    const points = _.values(_.groupBy(data, record => record['time'].ScalarValue))
      .map((records): Point => {
        const directions = records.reduce((rest, record) => {
          const directionIndex = +record['name'].ScalarValue.substr('wind_direction'.length, 1);
          return {
            ...rest,
            [WIND_DIRECTIONS[directionIndex]]: +record['value'].ScalarValue,
          };
        }, {} as Record<WindDirection, number>);
        return {
          time: moment.utc(records[0]['time'].ScalarValue).valueOf(),
          ...directions,
        };
      });
    return points;
  }

  renderData(ctx: Query.Context) {
    const { range } = this.props;
    const points = this.transformData(ctx.data);
    return <>
      {ctx.loading && <ChartLoader/>}
      <ResponsiveContainer>
        <AreaChart data={points} stackOffset="expand">
          <CartesianGrid strokeDasharray="3 3" />
          {timeAxis(range)}
          <YAxis domain={[0, 1]} tickFormatter={tick => `${tick * 100}%`} tickCount={5} width={40}/>
          <Legend />
          <Tooltip
            formatter={(value: number) => `${value}%`}
            labelFormatter={(value: number) => moment(value).format('llll')}/>
          {WIND_DIRECTIONS.map((direction, i) =>
            <Area key={direction} type="linear" stackId="s" dataKey={direction} fill={COLORS[i]} stroke={COLORS[i]} />
          )}
        </AreaChart>
      </ResponsiveContainer>
    </>;
  }

  render() {
    const { range, refreshKey } = this.props;

    const query = `
SELECT
time,
measure_name as name,
measure_value::double as value
FROM ${environment.databaseName}.records
WHERE time > ago(${range})
AND measure_name IN (${measures.map(measure => `'${measure}'`).join()})
ORDER BY time
      `.trim();

    return <Query query={query} refreshKey={refreshKey}>{this.renderData.bind(this)}</Query>;
  }
}
