import React from 'react';
import { Radar, RadarChart, PolarGrid, PolarAngleAxis, ResponsiveContainer, Legend } from 'recharts';
import { WIND_DIRECTIONS } from '../utils';

export const WindDirection: React.FC<{ wind: number[], legend?: React.ReactElement }> = ({ wind, legend }) => (
  <ResponsiveContainer>
    <RadarChart
      data={wind?.map((value, direction) => ({ direction: WIND_DIRECTIONS[direction], value }))}>
      <PolarGrid/>
      <PolarAngleAxis dataKey="direction" tickFormatter={label => label.length === 1 ? label : ''} />
      <Radar dataKey="value" stroke="#8884d8" fill="#8884d8" fillOpacity={0.8} />

      {legend && <Legend content={legend}/>}
    </RadarChart>
  </ResponsiveContainer>
  )
