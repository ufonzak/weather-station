/* eslint-disable jsx-a11y/anchor-is-valid */
import React from 'react';
import { SimpleChart } from './SimpleChart';
import { Range } from './utils';
import { WindChart } from './WindChart';
import { WindDirectionChart } from './WindDirection';

type AllCharts = 'wind' | 'direction' | 'temperature' | 'pressure' | 'humidity' | 'precipitation' | 'solar_voltage' | 'battery_voltage2';

type Props = unknown;

interface State {
  current: AllCharts;
  range: Range;
  refreshKey: number;
}

const CHART_LABELS = {
  'wind': 'Wind',
  'direction': 'Wind Direction',
  'temperature': 'Temperature',
  'pressure': 'Pressure',
  'humidity': 'Humidity',
  'precipitation': 'Precipitation',
  'solar_voltage': 'Solar Voltage',
  'battery_voltage2': 'Battery Voltage',
}
const UNITS: Partial<Record<AllCharts, string>> = {
  'temperature': '°C',
  'pressure': 'hPa',
  'humidity': '%',
  'precipitation': 'mm',
  'solar_voltage': 'V',
  'battery_voltage2': 'V',
}

const ChartPick: React.FC<{ current: AllCharts, onPick: (chart: AllCharts) => void }> = ({ current, onPick }) => (
  <div className="dropdown">
    <button className="btn btn-secondary dropdown-toggle" type="button" data-bs-toggle="dropdown" aria-expanded="false">
      {CHART_LABELS[current]}&ensp;
    </button>
    <ul className="dropdown-menu" aria-labelledby="dropdownMenuButton1">
      {(Object.keys(CHART_LABELS) as AllCharts[]).map(chart =>
        <li key={chart}>
          <a className="dropdown-item"
            onClick={() => onPick(chart)}>{CHART_LABELS[chart]}
          </a>
        </li>
      )}
    </ul>
  </div>
);

const RANGE_LABELS = {
  '1d': '1 day',
  '2d': '2 days',
  '7d': '1 week',
}

const RangePick: React.FC<{ range: Range, onPick: (chart: Range) => void }> = ({ range, onPick }) => (
  <div className="dropdown">
    <button className="btn btn-secondary dropdown-toggle" type="button" data-bs-toggle="dropdown" aria-expanded="false">
      {RANGE_LABELS[range]}&ensp;
    </button>
    <ul className="dropdown-menu" aria-labelledby="dropdownMenuButton1">
      {(Object.keys(RANGE_LABELS) as Range[]).map(range =>
        <li key={range}>
          <a className="dropdown-item"
            onClick={() => onPick(range)}>{RANGE_LABELS[range]}
          </a>
        </li>
      )}
    </ul>
  </div>
);

export class Charts extends React.Component<Props, State> {
  constructor(props: Props) {
    super(props);

    this.state = {
      current: 'wind',
      range: '1d',
      refreshKey: 0,
    };
  }

  render() {
    const { current, range, refreshKey } = this.state;
    return (<div className="charts mt-4">
      <div className="row">
        <div className="col-sm my-2 d-flex align-items-center">
          <span className="h2 mb-0">Charts</span>
        </div>
        <div className="col-md my-2 d-flex align-items-center controls">
          <div className="flex-grow-1"/>
          <ChartPick current={current} onPick={chart => this.setState({ current: chart })}/>
          <RangePick range={range} onPick={newRange => this.setState({ range: newRange })}/>
          <button type="button" className="btn btn-secondary" onClick={() => this.setState({ refreshKey: refreshKey + 1 })}>
            Refresh
          </button>
        </div>
      </div>
      <div className="row">
        <div className="col charts-column position-relative">
          {current === 'wind' && <WindChart range={range} refreshKey={refreshKey}/>}
          {current === 'direction' && <WindDirectionChart range={range} refreshKey={refreshKey}/>}
          {['temperature', 'pressure', 'humidity', 'precipitation', 'battery_voltage2', 'solar_voltage'].includes(current) &&
              <SimpleChart
                range={range}
                refreshKey={refreshKey}
                measurement={current}
                label={`${CHART_LABELS[current]} [${UNITS[current]}]`}
              />
            }
        </div>
      </div>
    </div>);
  }
}
