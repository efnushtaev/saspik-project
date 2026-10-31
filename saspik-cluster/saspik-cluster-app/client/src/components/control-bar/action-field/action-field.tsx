import { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { ActionButton } from './action-button/action-button';
import { CreateObjectModal } from '../../create-object-modal';
import { CreateUnitModal } from '../../create-unit-modal';
import { getUnitIdFromPathname, NAVIGATION_PATHS, withUnitPath } from '../../constants';

import './styles.css';

const cn = createCn('action-field');

type ActionFieldProps = {
  hideActionButton?: boolean;
};

export const ActionField = ({ hideActionButton = false }: ActionFieldProps) => {
  const navigate = useNavigate();
  const location = useLocation();
  const [isCreateOpen, setIsCreateOpen] = useState(false);
  const [isCreateUnitOpen, setIsCreateUnitOpen] = useState(false);

  const unitId = getUnitIdFromPathname(location.pathname);
  const isMainPage = location.pathname === '/';
  const defaultType = location.pathname.includes('/automation') ? 'device' : 'sensor';

  const handleClick = () => {
    navigate('/');
  };

  const handleObjectAdd = () => {
    setIsCreateOpen(true);
  };

  const handleCreated = () => {
    window.dispatchEvent(new CustomEvent('objects-updated'));
  };

  const handleUnitCreated = () => {
    window.dispatchEvent(new CustomEvent('units-updated'));
  };

  const isUnitListPage = [NAVIGATION_PATHS.sensor, NAVIGATION_PATHS.device].some(
    (path) => location.pathname === withUnitPath(path, unitId),
  );
  const addHidden = hideActionButton || !unitId || !isUnitListPage;

  return (
    <div className={cn({ hidden: hideActionButton })}>
      <div className={cn('left')}>
        <div className={cn('action-button', { hidden: hideActionButton })}>
          {!addHidden && <ActionButton type="" onClick={handleClick} />}
        </div>
        {!addHidden && (
          <button type="button" className={cn('add-object')} onClick={handleObjectAdd}>
            ＋ Добавить объект
          </button>
        )}
        {isMainPage && (
          <button
            type="button"
            className={cn('add-unit')}
            onClick={() => setIsCreateUnitOpen(true)}
          >
            ＋ Добавить юнит
          </button>
        )}
      </div>
      <CreateObjectModal
        open={isCreateOpen}
        unitId={unitId}
        defaultType={defaultType}
        onClose={() => setIsCreateOpen(false)}
        onCreated={handleCreated}
      />
      <CreateUnitModal
        open={isCreateUnitOpen}
        onClose={() => setIsCreateUnitOpen(false)}
        onCreated={handleUnitCreated}
      />
    </div>
  );
};
