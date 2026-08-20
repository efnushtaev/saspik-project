import { Route, Routes } from 'react-router-dom';
import { MainPage } from './main-page';
import { MonitoringPage } from './monitoring-page';
import { AutomationPage } from './automation-page';
import { RulesPage } from './rules-page';
import { InfoPage } from './info-page';
import { ObjectPage } from '../object-page';
import { UnitPage } from '../unit-page';

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
        <Route path="/rules" element={<RulesPage />} />
        <Route path="/info" element={<InfoPage />} />
        <Route path="/unit/:unitId/monitoring" element={<MonitoringPage />} />
        <Route path="/unit/:unitId/automation" element={<AutomationPage />} />
        <Route path="/unit/:unitId/rules" element={<RulesPage />} />
        <Route path="/unit/:unitId/info" element={<UnitPage />} />
        <Route path="/unit/:unitId/object/:objectId" element={<ObjectPage />} />
      </Routes>
    </div>
  );
};
