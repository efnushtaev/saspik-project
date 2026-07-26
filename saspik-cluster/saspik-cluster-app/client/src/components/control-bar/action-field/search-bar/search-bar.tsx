import { createCn } from 'bem-react-classname';
import { SearchOutlined } from '@ant-design/icons';

import './styles.css';

const cn = createCn('search-bar');

export const SearchBar = () => {
  return (
    <div className={cn()}>
      <div className={cn('content')}>Скоро здесь будет поиск</div>
      <SearchOutlined />
    </div>
  );
};
