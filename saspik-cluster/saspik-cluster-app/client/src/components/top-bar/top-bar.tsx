import { createCn } from 'bem-react-classname';
import { MenuOutlined } from '@ant-design/icons';

import { useTimestamp } from '../../hooks/use-timestamp';
import { formatDate } from '../../utils/format-date';

import './styles.css';

const cn = createCn('top-bar');

export const TopBar = () => {
  const { timestamp } = useTimestamp();

  return (
    <div className={cn()}>
      <div className={cn('actions')}>
        <MenuOutlined className={cn('icon')} />
        <div className={cn('right-side')}>
          <div className={cn('clock')}>{formatDate(timestamp)}</div>
        </div>
      </div>
      {/* <div className={cn('title')}>Личный кабинет</div> */}
    </div>
  );
};
