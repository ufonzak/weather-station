import { Query } from '../query';

export function findMeasure(rows: Query.Rows, measureName: string, valueName: string = 'value') {
  const measure = rows.find(record => record['name'].ScalarValue === measureName);
  return measure && parseFloat(measure[valueName].ScalarValue);
}

export type Range = 'today' | '1d' | '2d' | '7d';
