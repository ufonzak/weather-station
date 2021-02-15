import React from 'react';
import './App.scss';
import { Current } from './current';
import { WindChart, WindDirectionChart, SimpleChart } from './charts';

function App() {
  return (
    <div className="App">
      <Current/>
      <WindChart range="1d"/>
      <WindDirectionChart range="1d"/>
      <SimpleChart range="1d" measurement="temperature"/>
      <SimpleChart range="1d" measurement="pressure"/>
      <SimpleChart range="1d" measurement="humidity"/>
    </div>
  );
}

export default App;
