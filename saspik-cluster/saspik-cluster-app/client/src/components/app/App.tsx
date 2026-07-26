import { BrowserRouter as Router } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { useMobileDetection, useShowTabs } from '../../hooks';

import { TopBar } from '../top-bar';
import { ControlBar, ControlBarMobile } from '../control-bar';
import { PagesRoutes } from '../pages-routes/pages-routes';

import './styles.css';

const cn = createCn('app');

/**
 * Main application content component for mobile devices
 * Handles layout and composition of main application components for mobile
 */
const AppContentMobile = () => {
  const showTabs = useShowTabs();

  return (
    <div className={cn()} data-testid="app-container">
      <TopBar />
      <PagesRoutes showTabs={showTabs} />
      <ControlBarMobile showTabs={showTabs} />
    </div>
  );
};

/**
 * Main application content component for desktop devices
 * Handles layout and composition of main application components for desktop
 */
const AppContentDesktop = () => {
  const showTabs = useShowTabs();

  return (
    <div className={cn()} data-testid="app-container">
      <TopBar />
      <ControlBar showTabs={showTabs} />
      <PagesRoutes showTabs={showTabs} />
    </div>
  );
};

/**
 * Main application content component
 * Handles layout and composition of main application components
 * Switches between mobile and desktop layouts based on device detection
 */
const AppContent = () => {
  const isMobile = useMobileDetection();

  if (isMobile) {
    return <AppContentMobile />;
  }

  return <AppContentDesktop />;
};

/**
 * Main application component
 * Wraps the application content with the router
 */
export const App = () => {
  return (
    <Router>
      <AppContent />
    </Router>
  );
};
