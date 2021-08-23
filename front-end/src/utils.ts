export type RecursivePartial<T> = {
  [P in keyof T]?: RecursivePartial<T[P]>;
};
export type RecursiveReadonly<T> = {
  readonly [P in keyof T]: RecursiveReadonly<T[P]>;
};


export type UnPromisifiedObject<T> = {[k in keyof T]: UnPromisify<T[k]>};
export type UnPromisify<T> = T extends Promise<infer U> ? U : T;

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export type AnyFunction = (...args: any[]) => any;
// eslint-disable-next-line @typescript-eslint/no-explicit-any
export type AnyAsyncFunction = (...args: any[]) => Promise<any>;

export type AsyncReturnType<T extends AnyAsyncFunction> = UnPromisify<ReturnType<T>>;

export type WindDirection = 'N'|'NE'|'E'|'SE'|'S'|'SW'|'W'|'NW';

export const WIND_DIRECTIONS: WindDirection[] = [
  'N',
  'NE',
  'E',
  'SE',
  'S',
  'SW',
  'W',
  'NW',
];

export const getDataKey = (match: { params: { site: string } }) => {
  switch(match.params.site) {
    case 'bridal-falls-lz': return 'pemberton';
    default: return match.params.site || 'unknown';
  }
};
