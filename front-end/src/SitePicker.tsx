import React, { useLayoutEffect } from 'react';
import { useRouteMatch } from 'react-router';
import { Redirect, Route, Switch, useHistory } from 'react-router-dom';

const sites: Record<string, string> = {
  'woodside': 'Mt. Woodside',
  'bridal-falls-lz': 'Bridal Falls LZ',
};

export const SitePicker: React.FC = () => {
  const { params: { site } } = useRouteMatch<{ site: string }>();
  const router = useHistory();

  useLayoutEffect(() => {
    document.title = `${sites[site]} Weather Station`;
  }, [site]);

  return (
    <div className="site-picker mt-3">
      <div className="col-md">
        <select className="form-select form-select-lg h2"
          value={site}
          onChange={e => router.push(`/${e.target.value}`)}>
          {Object.entries(sites).map(([key, title]) =>
            <option key={key} value={key}>{title}</option>
          )}
        </select>
        <Switch>
          {Object.keys(sites).map(key => <Route key={key} path={`/${key}`}/>)}
          <Route><Redirect to={`/${Object.keys(sites)[0]}`}/></Route>
        </Switch>
      </div>
    </div>
  );
};
