import { Route, Routes } from 'react-router-dom';
import { MainPage } from './main-page';
import { MonitoringPage } from './monitoring-page';
import { AutomationPage } from './automation-page';
import { InfoPage } from './info-page';

import './styles.css';
import { createCn } from 'bem-react-classname';

const cn = createCn('pages-routes');

export const PagesRoutes = ({ showTabs = true }: { showTabs?: boolean }) => {
  return (
    <div className={cn({ 'tabs-visible': showTabs })}>
      <Routes>
        <Route path="/" element={<MainPage />} />
        <Route path="/monitoring" element={<MonitoringPage />} />
        <Route path="/automation" element={<AutomationPage />} />
        <Route path="/info" element={<InfoPage />} />
      </Routes>
    </div>
  );
};
