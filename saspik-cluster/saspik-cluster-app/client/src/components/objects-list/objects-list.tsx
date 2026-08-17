import { useState } from 'react';
import { createCn } from 'bem-react-classname';
import { useLocation, useNavigate } from 'react-router-dom';

import { ObjectsCard } from '../objects-card';
import { CreateObjectModal } from '../create-object-modal';
import { useObjectsListFetching } from '../../hooks/use-objects-list-fetching';
import { ObjectsListProps, ObjectItem } from './types';

import './styles.css';
import { NAVIGATION_PATHS, withUnitPath } from '../constants';
import { transformObjectToCard } from '../../utils/transform-object-to-card';

const cn = createCn('listing');

const toggleDeviceValue = (obj: ObjectItem): string | null => {
  const specEntry = obj.spec?.[0];
  if (!specEntry) return null;
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
  const [isCreateOpen, setIsCreateOpen] = useState(false);

  if (loading) {
    return <div className={'rotate-scale-up'} />;
  }

  if (error) {
    return <div className={cn()}>Ошибка загрузки: {error}</div>;
  }

  return (
    <div className={cn()}>
      {unitId && (
        <button
          type="button"
          className={cn('add-object')}
          onClick={() => setIsCreateOpen(true)}
        >
          ＋ Добавить объект
        </button>
      )}
      {objects.map((obj, index) => {
        const cardProps = transformObjectToCard(obj);
        const isDevice = type === 'device';

        return (
          <ObjectsCard
            key={index}
            {...cardProps}
            navigateTo={withUnitPath(NAVIGATION_PATHS[type], unitId)}
            onAction={isDevice ? () => {
              const newValue = toggleDeviceValue(obj);
              if (newValue) {
                sendCommand(obj.id, newValue);
                updateObjectValue(obj.id, obj.spec[0].key, newValue);
              }
            } : undefined}
            onOpen={() => navigate(unitId ? `/unit/${unitId}/object/${obj.id}` : `/object/${obj.id}`)}
          />
        );
      })}
      <CreateObjectModal
        open={isCreateOpen}
        unitId={unitId}
        defaultType={type}
        onClose={() => setIsCreateOpen(false)}
        onCreated={() => window.dispatchEvent(new CustomEvent('objects-updated'))}
      />
    </div>
  );
};
