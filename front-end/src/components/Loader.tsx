import React from 'react';
import classNames from 'classnames';

export type Props = React.DetailedHTMLProps<React.HTMLAttributes<HTMLDivElement>, HTMLDivElement>;

export const Loader: React.FC<Props> = ({ className, ...rest }) => (
  <div className={classNames('loader', className)}>
    <div className="spinner-border" role="status" {...rest}>
      <span className="visually-hidden">Loading...</span>
    </div>
  </div>
);
