import { createCn } from 'bem-react-classname';

import { ObjectsCard } from '../objects-card';
import { useObjectsListFetching } from '../../hooks/use-objects-list-fetching';
import { ObjectsListProps, ObjectItem } from './types';

import './styles.css';
import { NAVIGATION_PATHS } from '../constants';
import { transformObjectToCard } from '../../utils/transform-object-to-card';

const cn = createCn('listing');

const toggleDeviceValue = (obj: ObjectItem): string | null => {
  const specEntry = obj.spec?.[0];
  if (!specEntry) return null;
  if (specEntry.value == null) return '1';
  const currentValue = String(specEntry.value);
  if (specEntry.spec?.model === 'led') {
    return currentValue === 'OFF' ? 'ON' : 'OFF';
  }
  return currentValue === '0' ? '1' : '0';
};

export const ObjectsList = ({ type = 'sensor' }: ObjectsListProps) => {
  const { objects, loading, error, sendCommand, updateObjectValue } = useObjectsListFetching(type);

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

        return (
          <ObjectsCard
            key={index}
            {...cardProps}
            navigateTo={NAVIGATION_PATHS[type]}
            onAction={isDevice ? () => {
              const newValue = toggleDeviceValue(obj);
              if (newValue) {
                sendCommand(obj.id, newValue);
                updateObjectValue(obj.id, obj.spec[0].key, newValue);
              }
            } : undefined}
          />
        );
      })}
    </div>
  );
};
