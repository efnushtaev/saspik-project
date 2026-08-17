import { useParams } from 'react-router-dom';
import { ObjectsList } from '../objects-list';
import { BasePage } from './base-page';
import { useUnitPageHeader } from '../../hooks/use-unit-page-header';

export const MonitoringPage = () => {
  const { unitId = '' } = useParams();

  useUnitPageHeader(unitId);

  return (
    <BasePage>
      <ObjectsList type="sensor" unitId={unitId} />
    </BasePage>
  );
};
