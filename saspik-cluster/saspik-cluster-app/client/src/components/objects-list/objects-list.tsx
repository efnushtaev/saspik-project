import { createCn } from 'bem-react-classname';
import { useLocation, useNavigate } from 'react-router-dom';

import { ObjectsCard } from '../objects-card';
import { useObjectsListFetching } from '../../hooks/use-objects-list-fetching';
import { ObjectsListProps, ObjectItem } from './types';

import './styles.css';
import { NAVIGATION_PATHS, withUnitPath } from '../constants';
import { transformObjectToCard } from '../../utils/transform-object-to-card';

const cn = createCn('listing');

const toggleDeviceValue = (obj: ObjectItem, specEntry: ObjectItem['spec'][number]): string => {
  // Новый контракт device/#: канал передаётся в payload, значение бинарное ON/OFF.
  if ((obj.topic || '').startsWith('device/')) {
    return String(specEntry.value ?? '').toUpperCase() === 'ON' ? 'OFF' : 'ON';
  }
  if (specEntry.spec?.model === 'led') {
    return specEntry.value == null || String(specEntry.value) === 'OFF'
      ? 'ON'
      : 'OFF';
  }
  if (specEntry.value == null) return '1';
  const currentValue = String(specEntry.value);
  return currentValue === '0' ? '1' : '0';
};

export const ObjectsList = ({ type = 'sensor', unitId: unitIdProp }: ObjectsListProps) => {
  const { objects, loading, error, sendCommand, updateObjectValue } = useObjectsListFetching(
    type,
    unitIdProp,
  );
  const navigate = useNavigate();
  const location = useLocation();
  const unitId = unitIdProp || new URLSearchParams(location.search).get('id') || '';

  if (loading) {
    return <div className={'rotate-scale-up'} />;
  }

  if (error) {
    return <div className={cn()}>Ошибка загрузки: {error}</div>;
  }

  return (
    <div className={cn()}>
      {objects.map((obj, index) => {
        const cardProps = transformObjectToCard(obj);
        const isDevice = type === 'device';
        const controls = isDevice
          ? obj.spec.map((specEntry) => ({
              key: specEntry.key,
              value: specEntry.value == null ? '--' : String(specEntry.value),
              onToggle: () => {
                const newValue = toggleDeviceValue(obj, specEntry);
                sendCommand(obj.id, newValue, specEntry.key);
                updateObjectValue(obj.id, specEntry.key, newValue);
              },
            }))
          : undefined;
        return (
          <ObjectsCard
            key={index}
            {...cardProps}
            controls={controls}
            navigateTo={withUnitPath(NAVIGATION_PATHS[type], unitId)}
            onOpen={() => navigate(unitId ? `/unit/${unitId}/object/${obj.id}` : `/object/${obj.id}`)}
          />
        );
      })}
    </div>
  );
};
