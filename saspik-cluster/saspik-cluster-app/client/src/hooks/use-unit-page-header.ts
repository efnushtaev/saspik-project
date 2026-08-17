import { useCallback, useEffect, useState } from 'react';
import { mockApi, isMockMode } from '../components/mock-api';
import { usePageHeader } from '../components/top-bar/page-header-context';

interface UnitInfo {
  name: string;
}

/**
 * Sets the top-bar page header to the unit name for a given unit.
 * @param unitId - ID of the unit to display in the header (empty means no header)
 */
export const useUnitPageHeader = (unitId: string): void => {
  const [name, setName] = useState<string | null>(null);

  const fetchUnitName = useCallback(async () => {
    if (!unitId) {
      setName(null);
      return;
    }
    try {
      if (isMockMode()) {
        const res = await mockApi.getUnit(unitId);
        setName(res.unit.name);
      } else {
        const response = await fetch(`/api/v1/units/${unitId}`);
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data: { unit?: UnitInfo } = await response.json();
        setName(data.unit?.name ?? null);
      }
    } catch (err) {
      console.error('Error fetching unit name:', err);
      setName(null);
    }
  }, [unitId]);

  useEffect(() => {
    fetchUnitName();
  }, [fetchUnitName]);

  usePageHeader(name);
};
