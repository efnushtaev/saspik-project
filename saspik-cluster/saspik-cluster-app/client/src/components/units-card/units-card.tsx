import { createCn } from 'bem-react-classname';
import { useNavigate } from 'react-router-dom';

import './styles.css';

const cn = createCn('units-card');

type UnitsCardProps = {
  id: string;
  title: string;
  describe: string;
};

export const UnitsCard = ({ id, title, describe }: UnitsCardProps) => {
  const navigate = useNavigate();

  const handleClick = () => {
    navigate(`/monitoring?id=${id}`);
  };

  return (
    <div className={cn()} onClick={handleClick}>
      <div className={cn('content')}>
        <div className={cn('title')}>
          <div>{title}</div>
        </div>
        <div className={cn('describe')}>
          <div>{describe}</div>
        </div>
      </div>
      <div className={cn('picture')}>
        {title[0]}
        {describe[0]}
      </div>
    </div>
  );
};
