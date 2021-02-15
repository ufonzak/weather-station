import React from 'react';
import TimestreamQuery from 'aws-sdk/clients/timestreamquery';
import { client } from './client';

export declare namespace Query {
  export type Row = Record<string, TimestreamQuery.Datum>;
  export type Rows = Query.Row[];
}

interface Props {
  query?: string;
  onData?: (data: Query.Rows, refresh?: () => void) => React.ReactNode;
  onLoading?: () => React.ReactNode;
}

interface State {
  phase: 'loading' | 'data' | 'error' | 'dormant';
  rows: Query.Rows;
  error: string;
}

const defaultState: State = {
  phase: 'dormant', error: null, rows: null,
};

export class Query extends React.Component<Props, State> {
  private currentQuery: Promise<TimestreamQuery.Types.QueryResponse> = null;

  constructor(props: Props) {
    super(props);
    this.state = {
      ...defaultState,
    };
  }

  componentDidMount() {
    this.onRefresh();
  }

  componentWillUnmount() {
    this.currentQuery = null;
  }

  componentDidUpdate(prevProps: Props) {
    const { query } = this.props;
    if (query !== prevProps.query) {
      this.makeQuery(query);
    }
  }

  private makeQuery(query: string, nextToken?: string): void {
    this.currentQuery = null;

    if (!query) {
      this.setState(defaultState)
      return;
    }

    const queryPromise = client.query({
      QueryString: query,
      NextToken: nextToken,
    }).promise();
    this.currentQuery = queryPromise;

    queryPromise.then(response => {
      if (this.currentQuery !== queryPromise) {
        return;
      }

      this.onQueryResult(query, response)
    }, err => {
      if (this.currentQuery !== queryPromise) {
        return;
      }

      this.onQueryError(err);
    });

    this.setState({ error: null, phase: 'loading' });
  }

  private onQueryError(err: Error) {
    this.setState({ phase: 'error', error: err.message || String(err) });
  };

  private onQueryResult(query: string, response: TimestreamQuery.Types.QueryResponse) {
    if (response.NextToken) {
      this.makeQuery(query, response.NextToken);
    }

    const columns = response.ColumnInfo;
    const rows: Query.Rows = response.Rows.map(row => {
      const record: Query.Row = {};
      row.Data.forEach((data, i) => record[columns[i].Name || 'unknown'] = data);
      return record;
    });

    this.setState(prevState => ({
      rows: prevState.rows ? prevState.rows.concat(rows) : rows,
      phase: response.NextToken ? 'loading' : 'data',
    }))
  };

  private onRefresh = () => {
    const { query } = this.props;
    this.makeQuery(query);
  };

  render() {
    const { onData, onLoading } = this.props;
    const { phase, error, rows } = this.state;
    return <>
      {phase === 'loading' && !!onLoading && onLoading()}
      {phase === 'data' && !!onData && onData(rows, this.onRefresh)}
      {phase === 'error' && <div>This is broken. Error: {error}</div>}
    </>;
  }
}
