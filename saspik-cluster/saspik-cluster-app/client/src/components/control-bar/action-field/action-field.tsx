import { useNavigate } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { ActionButton } from './action-button/action-button';
import { SearchBar } from './search-bar';

import './styles.css';

const cn = createCn('action-field');

type ActionFieldProps = {
  hideActionButton?: boolean;
};

export const ActionField = ({ hideActionButton = false }: ActionFieldProps) => {
  const navigate = useNavigate();

  const handleClick = () => {
    navigate('/');
  };

  return (
    <div className={cn({ hidden: hideActionButton })}>
      <div className={cn('action-button', { hidden: hideActionButton })}>
        <ActionButton type="" onClick={handleClick} />
      </div>
      <SearchBar />
      <div className={cn('action-button', { hidden: hideActionButton })}>
        <ActionButton type="empty" />
      </div>
    </div>
  );
};
