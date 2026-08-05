import { useState } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { ActionButton } from './action-button/action-button';
import { SearchBar } from './search-bar';
import { CreateObjectModal } from '../../create-object-modal';

import './styles.css';

const cn = createCn('action-field');

type ActionFieldProps = {
  hideActionButton?: boolean;
};

export const ActionField = ({ hideActionButton = false }: ActionFieldProps) => {
  const navigate = useNavigate();
  const location = useLocation();
  const [isCreateOpen, setIsCreateOpen] = useState(false);

  const unitId = new URLSearchParams(location.search).get('id') || '';
  const defaultType = location.pathname === '/automation' ? 'device' : 'sensor';

  const handleClick = () => {
    navigate('/');
  };

  const handleObjectAdd = () => {
    setIsCreateOpen(true);
  };

  const handleCreated = () => {
    window.dispatchEvent(new CustomEvent('objects-updated'));
  };

  const addHidden = hideActionButton || !unitId;

  return (
    <div className={cn({ hidden: hideActionButton })}>
      <div className={cn('action-button', { hidden: hideActionButton })}>
        <ActionButton type="" onClick={handleClick} />
      </div>
      <SearchBar />
      <div className={cn('action-button', { hidden: addHidden })}>
        <ActionButton type="add" onClick={handleObjectAdd} />
      </div>
      <CreateObjectModal
        open={isCreateOpen}
        unitId={unitId}
        defaultType={defaultType}
        onClose={() => setIsCreateOpen(false)}
        onCreated={handleCreated}
      />
    </div>
  );
};
