import { useNavigate } from 'react-router-dom';

import { createCn } from 'bem-react-classname';
import { SunOutlined, EllipsisOutlined } from '@ant-design/icons';

import './styles.css';

export type ObjectsCardControl = {
  key: string;
  value: string;
  onToggle: () => void;
};

type ObjectsCardProps = {
  title: string;
  describe: string;
  values: string[];
  controls?: ObjectsCardControl[];
  meta?: string;
  navigateTo?: string;
  onAction?: () => void;
  onOpen?: () => void;
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

export const ObjectsCard = ({ title, describe, values, controls, meta, navigateTo = '/monitoring', onAction, onOpen }: ObjectsCardProps) => {
  const hasValues = values.length > 0;
  const navigate = useNavigate();

  const handleClick = () => {
    if (onAction) {
      onAction();
    } else {
      navigate(navigateTo);
    }
  };

  const handleOpen = (e: React.MouseEvent) => {
    e.stopPropagation();
    onOpen?.();
  };

  return (
    <div className={cn({ actionable: !!onAction })} onClick={handleClick}>
      <div className={cn('content')}>
        <div className={cn('content-top')}>
          <div>
            <div className={cn('title')}>{title}</div>
            <div className={cn('describe')}>{describe}</div>
            {meta && <div className={cn('meta')}>{meta}</div>}
          </div>
          <div className={cn('icons')}>
            <SunOutlined className={cn('icon')} />
            {onOpen && (
              <button type="button" className={cn('open')} onClick={handleOpen}>
                <EllipsisOutlined className={cn('open-icon')} />
              </button>
            )}
          </div>
        </div>
        {controls ? (
          <div className={cn('controls')}>
            {controls.map((control) => {
              const isOn =
                String(control.value).toUpperCase() === 'ON' ||
                String(control.value) === '1' ||
                String(control.value).toLowerCase() === 'true';
              return (
                <div key={control.key} className={cn('control')}>
                  <span className={cn('control-key')}>{control.key}</span>
                  <span className={cn('control-value')}>{control.value}</span>
                  <button
                    type="button"
                    className={cn('control-toggle', { on: isOn })}
                    onClick={(e) => {
                      e.stopPropagation();
                      control.onToggle();
                    }}
                  >
                    {isOn ? 'OFF' : 'ON'}
                  </button>
                </div>
              );
            })}
          </div>
        ) : (
          <div className={cn('values')}>
            {hasValues ? (
              values.map((v, i) => (
                <div key={i} className={cn('value')}><ValueDisplay value={v} /></div>
              ))
            ) : (
              <div className={cn('value', { loading: true })}>--</div>
            )}
          </div>
        )}
      </div>
    </div>
  );
};
