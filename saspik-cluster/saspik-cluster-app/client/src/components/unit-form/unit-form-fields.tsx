import { createCn } from 'bem-react-classname';

import { UnitFormValues } from './types';

import './styles.css';

const cn = createCn('unit-form');

interface UnitFormFieldsProps {
  values: UnitFormValues;
  disabled?: boolean;
  idReadonly?: boolean;
  variant?: 'stacked' | 'table';
  onChange: (patch: Partial<UnitFormValues>) => void;
}

export const UnitFormFields = ({
  values,
  disabled = false,
  idReadonly = false,
  variant = 'stacked',
  onChange,
}: UnitFormFieldsProps) => {
  const inputCls = variant === 'table' ? cn('table-input') : cn('input');

  const idInput = (
    <input
      className={inputCls}
      value={values.unitId}
      disabled={disabled}
      readOnly={idReadonly}
      onChange={e => onChange({ unitId: e.target.value })}
      placeholder="Например, unitId3"
    />
  );

  const nameInput = (
    <input
      className={inputCls}
      value={values.name}
      disabled={disabled}
      onChange={e => onChange({ name: e.target.value })}
      placeholder="Например, Теплица 2"
    />
  );

  const descInput = (
    <input
      className={inputCls}
      value={values.description}
      disabled={disabled}
      onChange={e => onChange({ description: e.target.value })}
      placeholder="Необязательно"
    />
  );

  if (variant === 'table') {
    return (
      <table className={cn('table')}>
        <tbody>
          <tr>
            <th className={cn('table-label')}>ID</th>
            <td>{idInput}</td>
          </tr>
          <tr>
            <th className={cn('table-label')}>Название</th>
            <td>{nameInput}</td>
          </tr>
          <tr>
            <th className={cn('table-label')}>Описание</th>
            <td>{descInput}</td>
          </tr>
        </tbody>
      </table>
    );
  }

  return (
    <>
      <label className={cn('field')}>
        <span className={cn('label')}>ID</span>
        {idInput}
      </label>

      <label className={cn('field')}>
        <span className={cn('label')}>Название</span>
        {nameInput}
      </label>

      <label className={cn('field')}>
        <span className={cn('label')}>Описание</span>
        {descInput}
      </label>
    </>
  );
};
