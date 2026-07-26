import { useNavigate } from 'react-router-dom';

import { createCn } from 'bem-react-classname';
import { SunOutlined } from '@ant-design/icons';

import './styles.css';

type ObjectsCardProps = {
  title: string;
  describe: string;
  values: string[];
  navigateTo?: string;
  onAction?: () => void;
};

const cn = createCn('objects-card');

const ValueDisplay = ({ value }: { value: string }) => {
  const match = value.match(/^(\d+)(\.\d+)?(\s+.*)?$/);
  if (!match) {
    return <>{value}</>;
  }

  const [, intPart, fracPart, unit] = match;

  return (
    <>
      <span className={cn('value-int')}>{intPart}</span>
      {fracPart && <span className={cn('value-frac')}>{fracPart}</span>}
      {unit && <span className={cn('value-unit')}>{unit}</span>}
    </>
  );
};

export const ObjectsCard = ({ title, describe, values, navigateTo = '/monitoring', onAction }: ObjectsCardProps) => {
  const hasValues = values.length > 0;
  const navigate = useNavigate();

  const handleClick = () => {
    if (onAction) {
      onAction();
    } else {
      navigate(navigateTo);
    }
  };

  return (
    <div className={cn({ actionable: !!onAction })} onClick={handleClick}>
      <div className={cn('content')}>
        <div className={cn('content-top')}>
          <div>
            <div className={cn('title')}>{title}</div>
            <div className={cn('describe')}>{describe}</div>
          </div>
          <SunOutlined className={cn('icon')} />
        </div>
        <div className={cn('values')}>
          {hasValues ? (
            values.map((v, i) => (
              <div key={i} className={cn('value')}><ValueDisplay value={v} /></div>
            ))
          ) : (
            <div className={cn('value', { loading: true })}>--</div>
          )}
        </div>
      </div>
    </div>
  );
};
