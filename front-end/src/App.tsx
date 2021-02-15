import React from 'react';
import './App.scss';
import { Current } from './current';
import { Charts } from './charts';

function App(): React.ReactElement {
  return (
    <div className="app container">
      <h1>Mt. Woodside</h1>
      <Current/>
      <Charts/>
      <div className="mt-4 mb-3">
        <strong>
          Disclaimer: The data are provided without any warranty. Do not rely on the data to assess the risk of flying.
        </strong>
      </div>
    </div>
  );
}

export default App;
