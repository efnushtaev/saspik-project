import { createCn } from 'bem-react-classname';
import { Tabs } from './tabs';

import './styles.css';

const cn = createCn('control-bar');

export const ControlBar = ({
  showTabs = true,
}: {
  showTabs?: boolean;
}) => {
  return (
    <div
      className={cn('wrapper', {
        'tabs-visible': showTabs,
      })}
    >
      <Tabs isVisible={showTabs} />
    </div>
  );
};
