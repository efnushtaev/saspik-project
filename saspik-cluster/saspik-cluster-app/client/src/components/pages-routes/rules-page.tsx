import { useParams } from 'react-router-dom';
import { RulesList } from '../rules-list';
import { BasePage } from './base-page';
import { useUnitPageHeader } from '../../hooks/use-unit-page-header';

export const RulesPage = () => {
  const { unitId = '' } = useParams();

  useUnitPageHeader(unitId);

  return (
    <BasePage>
      <RulesList unitId={unitId} />
    </BasePage>
  );
};