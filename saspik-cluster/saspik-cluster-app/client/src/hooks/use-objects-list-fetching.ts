import { useState, useEffect } from 'react';
import { useLocation } from 'react-router-dom';

import { mockApi, isMockMode } from '../components/mock-api';
import { ObjectItem, PageObjectType } from '../components/objects-list/types';

interface ListResponse {
  objects: ObjectItem[];
}

export interface UseObjectsListResult {
  objects: ObjectItem[];
  loading: boolean;
  error: string | null;
  sendCommand: (deviceId: string, value: string) => Promise<void>;
  updateObjectValue: (objectId: string, specKey: string, newValue: string) => void;
}

/**
 * Custom hook to fetch objects list data based on type from query parameters
 * @param type - Type of objects to fetch ('sensor' or 'device')
 * @returns Object containing objects list, loading state, and error state
 */
export const useObjectsListFetching = (type: PageObjectType = 'sensor'): UseObjectsListResult => {
  const [objects, setObjects] = useState<ObjectItem[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const location = useLocation();

  useEffect(() => {
    const loadingTimer = setTimeout(() => setLoading(false), 10000);

    const searchParams = new URLSearchParams(location.search);
    const unitId = searchParams.get('id') || undefined;

    const fetchObjects = async () => {
      try {
        if (isMockMode()) {
          const id = searchParams.get('id') || '';

          let data;
          if (type === 'sensor') {
            data = await mockApi.getSensorsList(id);
          } else {
            data = await mockApi.getAutomationsList(id);
          }
          setObjects(data.objects);
        } else {
          const response = await fetch(`/api/v1/objects/list/${type}`, {
            method: 'POST',
            headers: {
              'Content-Type': 'application/json',
            },
            body: JSON.stringify({ unitId }),
          });

          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }

          const data = await response.json() as ListResponse;
          setObjects(data.objects);
        }
      } catch (err) {
        setError(
          err instanceof Error ? err.message : 'An unknown error occurred'
        );
        console.error('Error fetching objects:', err);
      } finally {
        clearTimeout(loadingTimer);
        setLoading(false);
      }
    };

    fetchObjects();
    const interval = setInterval(fetchObjects, 5000);
    const handleObjectsUpdated = () => {
      fetchObjects();
    };
    window.addEventListener('objects-updated', handleObjectsUpdated);
    return () => {
      window.removeEventListener('objects-updated', handleObjectsUpdated);
      clearInterval(interval);
      clearTimeout(loadingTimer);
    };
  }, [location.search, type]);

  const sendCommand = async (deviceId: string, value: string) => {
    try {
      if (isMockMode()) {
        await mockApi.callCommand(deviceId, value);
      } else {
        const searchParams = new URLSearchParams(location.search);
        const unitId = searchParams.get('id') || undefined;
        const response = await fetch(`/api/v1/objects/command/${deviceId}`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ value, unitId }),
        });
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
      }
    } catch (err) {
      console.error('Error sending command:', err);
    }
  };

  const updateObjectValue = (objectId: string, specKey: string, newValue: string) => {
    setObjects(prev =>
      prev.map(obj =>
        obj.id === objectId
          ? {
              ...obj,
              spec: obj.spec.map(s =>
                s.key === specKey ? { ...s, value: newValue } : s,
              ),
            }
          : obj,
      ),
    );
  };

  return { objects, loading, error, sendCommand, updateObjectValue };
};
