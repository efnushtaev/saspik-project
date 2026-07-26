import { createCn } from 'bem-react-classname';
import { Tabs } from './tabs';
import { ActionField } from './action-field';
import { useLocation } from 'react-router-dom';

import './styles.css';

const cn = createCn('control-bar');

export const ControlBar = ({
  showTabs = true,
}: {
  showTabs?: boolean;
}) => {
  const { pathname } = useLocation();

  const isMainPage = pathname === '/';

  return (
    <div
      className={cn('wrapper', {
        'tabs-visible': showTabs,
      })}
    >
      <Tabs isVisible={showTabs} />
      <ActionField hideActionButton={isMainPage} />
    </div>
  );
};
