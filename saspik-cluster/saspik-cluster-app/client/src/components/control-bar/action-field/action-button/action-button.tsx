import { LeftOutlined } from '@ant-design/icons';
import { createCn } from 'bem-react-classname';

import './styles.css';

const cn = createCn('action-button');

type ActionButtonProps = {
  type: string;
  onClick?: () => void;
};

export const ActionButton = ({ type, onClick }: ActionButtonProps) => {
  /**
   *
   * @TODO: типы положить в маппу или енам
   */
  const renderIcon = () => {
    if (type === 'empty') {
      return <div className={cn('icon')} />;
    }

    if (type === 'add') {
      return <div className={cn('icon')}>+</div>;
    }

    return <LeftOutlined className={cn('icon')} />;
  };

  return (
    <div className={cn()} onClick={onClick}>
      <div className={cn('icon-wrapper')}>{renderIcon()}</div>
    </div>
  );
};
