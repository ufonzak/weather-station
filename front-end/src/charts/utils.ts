import { Query } from '../query';

export function findMeasure(rows: Query.Rows, measureName: string) {
  const measure = rows.find(record => record['name'].ScalarValue === measureName);
  return measure && parseFloat(measure['value'].ScalarValue);
}

export type Range = '1d' | '2d' | '7d';
