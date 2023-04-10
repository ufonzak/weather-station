import React, { useState } from 'react';
import { useParams } from 'react-router-dom';
import { Loader } from '../components';

const Woodside: React.FC = () => {
  const [loaded, setLoaded] = useState(false);

  if (window.location.protocol === 'https:') {
    return <p>
      You must access this page through insecure connection to see the
      camera. <a href={window.location.href.replace(/^https/, 'http')}>Take me there</a>.
    </p>;
  }

  return (<>
    {!loaded && <Loader>Camera loading...</Loader>}
    <img className="camera"
      src="http://woodsidelaunch.duckdns.org:100/"
      alt='Woodside Camera'
      onLoad={() => setLoaded(true)}/>
  </>);
}

export const Camera: React.FC = () => {
  const { site } = useParams<{ site: string }>();

  return (
    <div className="row mb-2">
      <div className="col">
        {site === 'woodside' && <Woodside/>}
      </div>
    </div>);
}
