import { createCn } from 'bem-react-classname';
import { Link, useLocation } from 'react-router-dom';
import './styles.css';
import {
  ApiOutlined,
  InfoCircleOutlined,
  LineChartOutlined,
} from '@ant-design/icons';
import { useEffect, useState } from 'react';

export const Tabs = ({
  isVisible: visibleProp = true,
}: {
  isVisible?: boolean;
}) => {
  const cn = createCn('tab-bar');
  const location = useLocation();
  const [isVisible, setIsVisible] = useState(visibleProp);

  const searchParams = new URLSearchParams(location.search);
  const currentId = searchParams.get('id');

  const createUrl = (path: string) => {
    if (currentId) {
      return `${path}?id=${currentId}`;
    }
    return path;
  };

  const isActive = (path: string) => {
    if (path === '/') {
      return location.pathname === '/';
    }
    return location.pathname.startsWith(path);
  };

  useEffect(() => {
    const frame = requestAnimationFrame(() => {
      setIsVisible(visibleProp);
    });

    return () => cancelAnimationFrame(frame);
  }, [visibleProp]);

  return (
    <div
      className={cn({
        visible: isVisible,
        'main-page': location.pathname === '/',
      })}
    >
      <Link
        to={createUrl('/monitoring')}
        className={cn('tab', {
          state: isActive('/monitoring') ? 'active' : false,
        })}
      >
        <div
          className={cn('icon', {
            state: isActive('/monitoring') ? 'active' : false,
          })}
        >
          <LineChartOutlined />
        </div>
        <div
          className={cn('label', {
            state: isActive('/monitoring') ? 'active' : false,
          })}
        >
          Мониторинг
        </div>
      </Link>
      <Link
        to={createUrl('/automation')}
        className={cn('tab', {
          state: isActive('/automation') ? 'active' : false,
        })}
      >
        <div
          title="Автоматика"
          className={cn('icon', {
            state: isActive('/automation') ? 'active' : false,
          })}
        >
          <ApiOutlined />
        </div>
        <div
          className={cn('label', {
            state: isActive('/automation') ? 'active' : false,
          })}
        >
          Автоматика
        </div>
      </Link>
      <Link
        to={createUrl('/info')}
        className={cn('tab', { state: isActive('/info') ? 'active' : false })}
      >
        <div
          title="Информация"
          className={cn('icon', {
            state: isActive('/info') ? 'active' : false,
          })}
        >
          <InfoCircleOutlined />
        </div>
        <div
          className={cn('label', {
            state: isActive('/info') ? 'active' : false,
          })}
        >
          Информация
        </div>
      </Link>
    </div>
  );
};
