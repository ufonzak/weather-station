import React from 'react';
import _ from 'lodash';
import moment from 'moment';
import { environment } from '../environment';
import { Query } from '../query';
import { WindDirection } from './WindDirection';

const RECORDS_BACK = 3;

const measures = [
  'wind_speed_10m_avg',
  'wind_speed_10m_max',
  'temperature',
  'pressure',
  'humidity',
  ... _.range(8).map(i => `wind_direction${i}_10m`),
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

const Age: React.FC<{ row: Query.Row }> = ({ row }) => {
  if (!row) {
    return <>N/A</>;
  }
  const ts = moment.utc(row['time'].ScalarValue);
  const ageMinutes = moment.utc().diff(ts, 'minutes');
  return <>{ageMinutes} minutes</>;
}

export class Current extends React.Component {
  renderData(data: Query.Rows) {
    if (!data.length) {
      return <>Data not available</>;
    }

    const recordGroups = _.values(_.groupBy(data, record => record['time'].ScalarValue));
    const [records1, records2, records3] = recordGroups;

    return <>
      <div>Age <Age row={records1[0]}/></div>

      <div>Wind Average (10 min) {getRecord(records1, 'wind_speed_10m_avg')} km/h</div>
      <div>Wind Gust (10 min) {getRecord(records1, 'wind_speed_10m_max')} km/h</div>

      <div>Pressure {getRecord(records1, 'pressure')} hPa</div>
      <div>Temperature {getRecord(records1, 'temperature')} °C</div>
      <div>Temperature {getRecord(records2, 'temperature')} °C</div>
      <div>Temperature {getRecord(records3, 'temperature')} °C</div>
      <div>Relative Humidity {getRecord(records1, 'humidity')} %</div>

      <WindDirection wind={getWindDirections(records1)}/>
      <WindDirection wind={getWindDirections(records2)}/>
      <WindDirection wind={getWindDirections(records3)}/>
    </>;
  }

  render() {
    return <div className="current">
      <Query query={query} onData={data => this.renderData(data)}/>
    </div>;
  }
}
