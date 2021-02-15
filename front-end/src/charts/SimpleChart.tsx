import React from 'react';
import _ from 'lodash';
import moment from 'moment';
import {  AreaChart, CartesianGrid, Legend, Line, LineChart, ResponsiveContainer, Tooltip, YAxis } from 'recharts';

import { environment } from '../environment';
import { Query } from '../query';
import { Range } from './utils';
import { timeAxis } from './TimeAxis';

interface Point {
  time: number;
  value: number;
}

interface Props {
  measurement: string;
  range: Range;
}

export class SimpleChart extends React.Component<Props> {
  transformData(data: Query.Rows): Point[] { // TODO: memoize
    return data.map((record): Point => ({
      time: moment.utc(record['time'].ScalarValue).valueOf(),
      value: parseFloat(record['value'].ScalarValue),
    }));
  }

  renderData(data: Query.Rows) {
    const { range, measurement } = this.props;
    const points = this.transformData(data);
    return <ResponsiveContainer width={1000} height={500}>
      <LineChart data={points}>
        <CartesianGrid strokeDasharray="3 3" />
        {timeAxis(range)}
        <YAxis domain={['auto', 'auto']}/>
        <Legend formatter={() => measurement}/>
        <Tooltip
          labelFormatter={(value: number) => moment(value).format('llll')}/>
        <Line type="linear" dataKey="value" stroke="#8884d8" />
      </LineChart>
    </ResponsiveContainer>;
  }

  render() {
    const { range, measurement } = this.props;

    const query = `
SELECT
time,
measure_name as name,
measure_value::double as value
FROM ${environment.databaseName}.records
WHERE time > ago(${range})
AND measure_name = '${measurement}'
ORDER BY time
      `.trim();

    return <div className="current">
      <Query query={query} onData={data => this.renderData(data)}/>
    </div>;
  }
}
