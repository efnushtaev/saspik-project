import { createCn } from 'bem-react-classname';
import React from 'react';

import './styles.css';

const cn = createCn('page');

interface BasePageProps {
  children: React.ReactNode;
}

export const BasePage = ({ children }: BasePageProps) => {
  return (
    <div className={cn()}>
      <div className={cn('content')}>{children}</div>
    </div>
  );
};
