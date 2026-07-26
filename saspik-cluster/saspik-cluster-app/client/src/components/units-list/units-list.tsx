import { useState, useEffect } from 'react';
import { createCn } from 'bem-react-classname';

import { UnitsCard } from '../units-card';
import { mockApi, isMockMode } from '../mock-api';

import './styles.css';

const cn = createCn('listing');

interface Unit {
  id: string;
  name: string;
  description?: string;
  objects: unknown[];
  rules: unknown[];
}

interface GetUnitsListResponse {
  units: Unit[];
}

export const UnitsList = () => {
  const [units, setUnits] = useState<Unit[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    const fetchUnits = async () => {
      try {
        if (isMockMode()) {
          // Use mock data
          const data = await mockApi.getUnitsList();
          setUnits(data.units);
        } else {
          // Use real API
          const response = await fetch('/api/v1/units/list');
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
          const data: GetUnitsListResponse = await response.json();
          if (Array.isArray(data?.units)) {
            setUnits(data.units);
          } else {
            console.error('Unexpected API response:', data);
            setError('Invalid API response format');
          }
        }
      } catch (err) {
        setError(err instanceof Error ? err.message : 'An unknown error occurred');
        console.error('Error fetching units:', err);
      } finally {
        setLoading(false);
      }
    };

    fetchUnits();
  }, []);

  if (loading) {
    return <div className={'rotate-scale-up'} />;
  }

  if (error) {
    return <div className={cn()}>Ошибка загрузки: {error}</div>;
  }

  return (
    <div className={cn()}>
      {units.map((unit) => (
        <UnitsCard
          key={unit.id}
          id={unit.id}
          title={unit.name}
          describe={unit.description || ''}
        />
      ))}
    </div>
  );
};
