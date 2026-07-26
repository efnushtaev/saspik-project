import { ObjectsList } from '../objects-list';
import { BasePage } from './base-page';

export const MonitoringPage = () => {
  return (
    <BasePage>
      <ObjectsList type="sensor" />
    </BasePage>
  );
};
