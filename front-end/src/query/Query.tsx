import React from 'react';
import TimestreamQuery from 'aws-sdk/clients/timestreamquery';
import { client } from './client';

export declare namespace Query {
  export type Row = Record<string, TimestreamQuery.Datum>;
  export type Rows = Query.Row[];
  export interface Context {
    loading: boolean;
    data?: Query.Rows;
    refresh: () => void;
  }
}


interface Props {
  refreshKey?: string | number;
  query?: string;
  children: (ctx?: Query.Context) => React.ReactElement;
}

interface State {
  rows: Query.Rows;
  error: string;
  loading: boolean;
}

const defaultState: State = {
  error: null,
  rows: null,
  loading: false,
};

export class Query extends React.Component<Props, State> {
  private currentQuery: Promise<TimestreamQuery.Types.QueryResponse> = null;
  private loadedRows: Query.Rows = null;

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
    const { query, refreshKey } = this.props;
    if (query !== prevProps.query || refreshKey !== prevProps.refreshKey) {
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

    if (!nextToken) {
      this.loadedRows = [];
      this.setState({ error: null, loading: true });
    }
  }

  private onQueryError(err: Error) {
    this.setState({ error: err.message || String(err) || 'Unknown Error', loading: false });
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

    this.loadedRows = this.loadedRows.concat(rows);

    if (!response.NextToken) {
      this.setState({
        rows: this.loadedRows,
        loading: false,
      });
      this.loadedRows = null;
    }
  };

  private onRefresh = () => {
    const { query } = this.props;
    this.makeQuery(query);
  };

  render() {
    const { children } = this.props;
    const { error, rows, loading } = this.state;
    return <>
      {!error && children({ loading, data: rows, refresh: this.onRefresh })}
      {!!error && <div>This is broken. Error: {error}</div>}
    </>;
  }
}
