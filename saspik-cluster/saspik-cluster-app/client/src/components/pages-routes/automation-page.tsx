import { ObjectsList } from '../objects-list';
import { BasePage } from './base-page';

export const AutomationPage = () => {
  return (
    <BasePage>
      <ObjectsList type="device" />
    </BasePage>
  );
};
