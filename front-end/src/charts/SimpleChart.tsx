import React from 'react';
import moment from 'moment';
import { RouteComponentProps, withRouter } from 'react-router-dom';
import { CartesianGrid, Legend, Line, LineChart, ResponsiveContainer, Tooltip, YAxis } from 'recharts';

import { environment } from '../environment';
import { Query } from '../query';
import { Range } from './utils';
import { timeAxis } from './TimeAxis';
import { ChartLoader } from './ChartLoader';
import { getDataKey } from '../utils';

interface Point {
  time: number;
  value: number;
}

interface Props extends RouteComponentProps<{ site: string }> {
  refreshKey?: number | string;
  measurement: string;
  range: Range;
  label?: string;
}

class SimpleChartBase extends React.Component<Props> {
  transformData(data: Query.Rows): Point[] { // TODO: memoize
    return data?.map((record): Point => ({
      time: moment.utc(record['time'].ScalarValue).valueOf(),
      value: parseFloat(record['value'].ScalarValue),
    }));
  }

  renderData(ctx: Query.Context) {
    const { range, measurement, label } = this.props;
    const points = this.transformData(ctx.data);
    return <>
      {ctx.loading && <ChartLoader/>}
      <ResponsiveContainer>
        <LineChart data={points}>
          <CartesianGrid strokeDasharray="3 3" />
          {timeAxis(range)}
          <YAxis domain={['auto', 'auto']} width={40}/>
          <Legend formatter={() =>  label || measurement}/>
          <Tooltip
            labelFormatter={(value: number) => moment(value).format('llll')}/>
          <Line type="linear" dataKey="value" stroke="#8884d8" dot={null} />
        </LineChart>
      </ResponsiveContainer>
    </>;
  }

  render() {
    const { range, refreshKey, measurement, match } = this.props;

    const query = `
SELECT
time,
measure_name as name,
measure_value::double as value
FROM ${environment.databaseName}.records
WHERE station = '${getDataKey(match)}' AND time > ago(${range})
AND measure_name = '${measurement}'
ORDER BY time
      `.trim();

    return <Query query={query} refreshKey={refreshKey}>{this.renderData.bind(this)}</Query>;
  }
}

export const SimpleChart = withRouter(SimpleChartBase);
