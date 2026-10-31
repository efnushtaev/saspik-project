import { Link } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { useUnitName } from '../../hooks/use-unit-name';

const cn = createCn('top-bar');

interface BreadcrumbsProps {
  unitId: string;
}

export const Breadcrumbs = ({ unitId }: BreadcrumbsProps) => {
  const unitName = useUnitName(unitId);

  return (
    <nav className={cn('breadcrumbs')} aria-label="Хлебные крошки">
      <Link to="/" className={cn('breadcrumbs-link')}>
        Все юниты
      </Link>
      <span className={cn('breadcrumbs-separator')} aria-hidden="true">
        →
      </span>
      <span className={cn('breadcrumbs-current')}>{unitName ?? unitId}</span>
    </nav>
  );
};
