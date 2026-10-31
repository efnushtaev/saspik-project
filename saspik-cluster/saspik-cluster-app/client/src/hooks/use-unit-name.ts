import { useEffect, useState } from 'react';
import { mockApi, isMockMode } from '../components/mock-api';

interface UnitInfo {
  name: string;
}

/**
 * Loads a unit name by its ID.
 * @param unitId - ID of the unit (empty means no name)
 * @returns Unit name or null while loading / on error
 */
export const useUnitName = (unitId: string): string | null => {
  const [name, setName] = useState<string | null>(null);

  useEffect(() => {
    let cancelled = false;

    if (!unitId) {
      setName(null);
      return () => {
        cancelled = true;
      };
    }

    const fetchUnitName = async () => {
      try {
        if (isMockMode()) {
          const res = await mockApi.getUnit(unitId);
          if (!cancelled) setName(res.unit.name);
        } else {
          const response = await fetch(`/api/v1/units/${unitId}`);
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
          const data: { unit?: UnitInfo } = await response.json();
          if (!cancelled) setName(data.unit?.name ?? null);
        }
      } catch (err) {
        console.error('Error fetching unit name:', err);
        if (!cancelled) setName(null);
      }
    };

    fetchUnitName();

    return () => {
      cancelled = true;
    };
  }, [unitId]);

  return name;
};
