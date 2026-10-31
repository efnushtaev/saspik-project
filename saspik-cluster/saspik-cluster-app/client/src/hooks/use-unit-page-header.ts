import { usePageHeader } from '../components/top-bar/page-header-context';
import { useUnitName } from './use-unit-name';

/**
 * Sets the top-bar page header to the unit name for a given unit.
 * @param unitId - ID of the unit to display in the header (empty means no header)
 */
export const useUnitPageHeader = (unitId: string): void => {
  usePageHeader(useUnitName(unitId));
};
