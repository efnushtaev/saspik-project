import { createCn } from 'bem-react-classname';

import { useShowTabs } from '../../hooks';

import { TopBar } from '../top-bar';
import { ControlBar } from '../control-bar/control-bar';
import { PagesRoutes } from '../pages-routes/pages-routes';

const cn = createCn('app');

/**
 * Main application content component for mobile devices
 * Handles layout and composition of main application components for mobile
 */
export const AppContentMobile = () => {
  const showTabs = useShowTabs();

  return (
    <div className={cn()} data-testid="app-container">
      <TopBar />
      
      <ControlBar showTabs={showTabs} />
      
      <PagesRoutes showTabs={showTabs} />
    </div>
  );
};