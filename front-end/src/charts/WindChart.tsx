import React from 'react';
import _ from 'lodash';
import moment from 'moment';
import { CartesianGrid, DotProps, Legend, Line, LineChart, ResponsiveContainer, Tooltip, TooltipProps, YAxis } from 'recharts';
import {  withRouter } from 'react-router-dom';

import { environment } from '../environment';
import { Query } from '../query';
import { getDataKey, WIND_DIRECTIONS, TopRouteProps } from '../utils';
import { findMeasure, Range } from './utils';
import { timeAxis } from './TimeAxis';
import { ChartLoader } from './ChartLoader';

const measures = [
  'wind_speed_1h_avg',
  'wind_speed_1h_max',
  ..._.range(8).map(i => `wind_direction${i}_1h`),
];

interface Point {
  time: number;
  avg: number;
  max: number;
  direction: number;
}

interface Props extends TopRouteProps {
  refreshKey?: number | string;
  range: Range;
}

const CustomTooltip: React.FC<TooltipProps<number, string>> = ({ active, payload }) => {
  if (!active || !payload) {
    return null;
  }

  const point = payload[0].payload as Point;
  return (
    <div className="custom-tooltip">
      <div>{moment(point.time).format('llll')}</div>
      <div>Average: {Math.ceil(point.avg)} km/h</div>
      <div>Gust: {Math.ceil(point.max)} km/h</div>
      <div>Direction: {WIND_DIRECTIONS[point.direction]}</div>
    </div>
  );
};

const ARROW_SIZE = 30;

const Direction: React.FC<{ payload: Point } & DotProps> = ({ cx, cy, payload, key }) => (
  <svg key={key} x={cx - ARROW_SIZE / 2} y={cy - ARROW_SIZE / 2} width={ARROW_SIZE} height={ARROW_SIZE} fill="#505050" viewBox="0 0 40 40">
    <g transform={`rotate(${90 + payload.direction * 45} 20 20)`}>
      <path transform={`translate(7 0)`} d="M16.9,10.9c-0.6-0.6-1.7-0.6-2.3,0c-0.6,0.6-0.6,1.7,0,2.3l5.1,5.2H1.6C0.7,18.4,0,19.1,0,20
        c0,0.9,0.7,1.6,1.6,1.6h18.1l-5.2,5.2c-0.6,0.6-0.6,1.7,0,2.3c0.7,0.6,1.7,0.6,2.3,0L26,20l0,0L16.9,10.9z"/>
    </g>
  </svg>
);

class WindChartBase extends React.Component<Props> {
  transformData(data: Query.Rows): Point[] { // TODO: memoize
    const points = _.values(_.groupBy(data, record => record['time'].ScalarValue))
      .map((records): Point => {
        const directions = records.filter(record => record['name'].ScalarValue.startsWith('wind_direction'));
        const dominantDirection = _.maxBy(directions, record => +record['value'].ScalarValue);
        const directionIndex = +dominantDirection['name'].ScalarValue.substr('wind_direction'.length, 1)
        return {
          time: moment.utc(records[0]['time'].ScalarValue).valueOf(),
          avg: findMeasure(records, 'wind_speed_1h_avg'),
          max: findMeasure(records, 'wind_speed_1h_max', 'value_max'),
          direction: directionIndex,
        };
      });
    return points;
  }

  renderData(ctx: Query.Context) {
    const { range } = this.props;
    const points = this.transformData(ctx.data);
    return <>
      {ctx.loading && <ChartLoader/>}
      <ResponsiveContainer className="wind-chart">
        <LineChart data={points}>
          <CartesianGrid strokeDasharray="3 3" />
          {timeAxis(range)}
          <YAxis domain={[0, 'auto']} tickCount={8} width={30}/>
          <Tooltip content={CustomTooltip} />
          <Legend formatter={key => WindChartBase.labels[key]}/>
          <Line type="linear" dataKey="avg" stroke="#8884d8" dot={Direction} />
          <Line type="linear" dataKey="max" stroke="#F5CD00" dot={null} />
        </LineChart>
      </ResponsiveContainer>
    </>;
  }

  static labels: Record<string, string> = {
    'avg': 'Average Wind [km/h]',
    'max': 'Gust [km/h]',
  }

  static timeExpressions = {
    '1d': 'time',
    '2d': 'bin(time, 2h)',
    '7d': 'bin(time, 4h)',
  };
  static binCorrections = {
    '1d': '',
    '2d': ' + 1h',
    '7d': ' + 2h',
  };

  render() {
    const { range, refreshKey, match } = this.props;

    const timeExpression = WindChartBase.timeExpressions[range];
    const binCorrection = WindChartBase.binCorrections[range];
    const query = `
    WITH base_data AS (
      SELECT
      array_agg(time)[1] - 30m as time,
      measure_name as name,
      array_agg(measure_value::double)[1] as value
      FROM ${environment.databaseName}.records
      WHERE station = '${getDataKey(match)}' AND time > ago(${range})
      AND measure_name IN (${measures.map(measure => `'${measure}'`).join()})
      GROUP BY bin(time, 1h), measure_name
    )
    SELECT ${timeExpression}${binCorrection} as time, name, avg(value) as value, max(value) as value_max
    FROM base_data
    GROUP BY ${timeExpression}, name
    ORDER BY ${timeExpression}
      `.trim();

    return <Query query={query} refreshKey={refreshKey}>{this.renderData.bind(this)}</Query>;
  }
}

export const WindChart = withRouter(WindChartBase);
