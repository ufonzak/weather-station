import React from 'react';
import _ from 'lodash';
import moment from 'moment';
import { environment } from '../environment';
import { Query } from '../query';
import { WindDirection } from './WindDirection';
import { WIND_DIRECTIONS } from '../utils';
import { Loader } from '../components';

const RECORDS_BACK = 3;

const measures = [
  'wind_speed_10m_avg',
  'wind_speed_10m_max',
  'temperature',
  'pressure',
  'humidity',
  ..._.range(8).map(i => `wind_direction${i}_10m`),
];

const query = `
SELECT
time,
measure_name,
measure_value::double as value
FROM ${environment.databaseName}.records
WHERE time > ago(12h)
AND measure_name IN (${measures.map(measure => `'${measure}'`).join()})
ORDER BY time DESC, measure_name ASC
LIMIT ${measures.length * RECORDS_BACK}
`

function getRecord(data: Query.Rows, measure: string): number {
  const record = data.find(record => record['measure_name'].ScalarValue === measure);
  if (!record) {
    return null;
  }
  return parseFloat(record['value'].ScalarValue);
}

function getWindDirections(data: Query.Rows): number[] {
  return data.filter(record => record['measure_name'].ScalarValue.startsWith('wind_direction'))
    .map(record => parseFloat(record['value'].ScalarValue));
}
function getDominantDirection(windDirections: number[]): string {
  const dominantDirection = windDirections.reduce((best, current, i, array) => current > array[best] ? i : best, 0);
  return WIND_DIRECTIONS[dominantDirection];
}

const Age: React.FC<{ row: Query.Row }> = ({ row }) => {
  const ts = moment.utc(row['time'].ScalarValue);
  const diff = moment.duration(moment.utc().diff(ts));

  return <>{diff.hours() > 0 && <>{diff.hours()}&nbsp;h</>} {diff.minutes()}&nbsp;m</>;
}

 // TODO: render every minute
export class Current extends React.Component {
  private renderInterval: ReturnType<typeof setInterval>;

  componentDidMount() {
    this.renderInterval = setInterval(() => this.forceUpdate(), 60 * 1000);
  }

  componentWillUnmount() {
    clearInterval(this.renderInterval);
  }

  renderData(ctx: Query.Context) {
    const recordGroups = _.values(_.groupBy(ctx.data, record => record['time'].ScalarValue));
    const windDirections = recordGroups.map(getWindDirections);

    return <>
      <div className="row">
        <div className="col position-relative">
          <div className="d-flex align-items-center mb-2">
            <span className="h2 mb-0">Recent</span>
            <div className="flex-grow-1"/>
            {ctx.loading && <Loader/>}
            <button type="button" className="btn btn-secondary ms-3" onClick={ctx.refresh}>
              Refresh
            </button>
          </div>

          <table className="table">
            <tbody>
              <tr>
                <th scope="row">Age</th>
                {recordGroups.map((records, i) => <td key={i}><Age row={records[0]}/></td>)}
              </tr>
              <tr>
                <th scope="row">Wind Average (10&nbsp;min)</th>
                {recordGroups.map((records, i) => <td key={i}>{getRecord(records, 'wind_speed_10m_avg')} km/h</td>)}
              </tr>
              <tr>
                <th scope="row">Wind Gust (10&nbsp;min)</th>
                {recordGroups.map((records, i) => <td key={i}>{getRecord(records, 'wind_speed_10m_max')} km/h</td>)}
              </tr>
              <tr>
                <th scope="row">Dominant Direction</th>
                {recordGroups.map((records, i) => <td key={i}>{getDominantDirection(windDirections[i])}</td>)}
              </tr>
              <tr>
                <th scope="row">Temperature</th>
                {recordGroups.map((records, i) => <td key={i}>{getRecord(records, 'temperature').toFixed(1)} °C</td>)}
              </tr>
              <tr>
                <th scope="row">Humidity</th>
                {recordGroups.map((records, i) => <td key={i}>{getRecord(records, 'humidity')} %</td>)}
              </tr>
              <tr>
                <th scope="row">Pressure</th>
                {recordGroups.map((records, i) => <td key={i}>{getRecord(records, 'pressure')} hPa</td>)}
              </tr>
            </tbody>
          </table>
        </div>
        <div className="row wind-direction">
          {recordGroups.map((records, i) => (
            <div key={i} className="col-md chart">
              <WindDirection wind={windDirections[i]} legend={<><Age row={records[0]}/> ago</>}/>
            </div>
          ))}
          {recordGroups.length === 0 && <div className="col spacer"/>}
        </div>
      </div>
    </>;
  }

  render() {
    return <div className="current">
      <Query query={query}>{this.renderData.bind(this)}</Query>
    </div>;
  }
}
