import React from 'react';
import moment from 'moment';
import { Range } from './utils';
import { XAxis } from 'recharts';

const tickFormats = {
  '1d': 'HH:mm',
  '2d': 'dd HH:mm',
  '7d': 'dd HH:mm',
};

// recharts do not support wrapping components for some reason
// https://github.com/recharts/recharts/issues/412
export const timeAxis = (range: Range) => {
  const tickFormat = tickFormats[range];
  return (<XAxis
    padding={{right: 25}}
    scale="time"
    dataKey="time"
    type="number"
    domain={['auto', 'auto']}
    tickFormatter={timeStr => moment(timeStr).format(tickFormat)}
  />);
}
