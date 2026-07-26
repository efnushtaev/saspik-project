import { createCn } from 'bem-react-classname';
import { Tabs } from './tabs';
import { ActionField } from './action-field';
import { useLocation } from 'react-router-dom';

import './styles.css';

const cn = createCn('control-bar');

export const ControlBarMobile = ({
  showTabs = true,
}: {
  showTabs?: boolean;
}) => {
  const { pathname } = useLocation();

  const isMainPage = pathname === '/';

  return (
    <div
      className={cn('mobile-wrapper', {
        'tabs-visible': showTabs,
      })}
    >
      <ActionField hideActionButton={isMainPage} />
      {!isMainPage ? <Tabs isVisible={showTabs} /> : null}
    </div>
  );
};