import React from 'react';
import {
  BrowserRouter as Router,
  Switch,
  Route,
  Redirect,
} from 'react-router-dom';
import { Current } from './current';
import { Charts } from './charts';
import { SitePicker } from './SitePicker';

function App(): React.ReactElement {
  return (
    <Router>
      <Switch>
        <Route path="/:site">
          <div className="app container">
            <SitePicker/>
            <Current/>
            <Charts/>
            <div className="mt-4 mb-3">
              <strong>
                Disclaimer: The data are provided without any warranty. Do not rely on the data to assess the risk of flying.
              </strong>
            </div>
          </div>
        </Route>
        <Route><Redirect to="/woodside"/></Route>
      </Switch>
    </Router>
  );
}

export default App;
