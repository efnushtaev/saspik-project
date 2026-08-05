import { createCn } from 'bem-react-classname';

import { ObjectFormValues, SpecRow } from './types';

import './styles.css';

const cn = createCn('object-form');

interface ObjectFormFieldsProps {
  values: ObjectFormValues;
  unitId: string;
  disabled?: boolean;
  idReadonly?: boolean;
  variant?: 'stacked' | 'table';
  onChange: (patch: Partial<ObjectFormValues>) => void;
  onSpecChange: (index: number, patch: Partial<SpecRow>) => void;
  onSpecAdd: () => void;
  onSpecRemove: (index: number) => void;
}

export const ObjectFormFields = ({
  values,
  unitId,
  disabled = false,
  idReadonly = false,
  variant = 'stacked',
  onChange,
  onSpecChange,
  onSpecAdd,
  onSpecRemove,
}: ObjectFormFieldsProps) => {
  const topic = `${values.type}/${unitId}/${values.objectId}`;
  const inputCls = variant === 'table' ? cn('table-input') : cn('input');

  const nameInput = (
    <input
      className={inputCls}
      value={values.name}
      disabled={disabled}
      onChange={e => onChange({ name: e.target.value })}
      placeholder="Например, DHT22"
    />
  );

  const typeSelect = (
    <select
      className={inputCls}
      value={values.type}
      disabled={disabled}
      onChange={e => onChange({ type: e.target.value as ObjectFormValues['type'] })}
    >
      <option value="sensor">sensor</option>
      <option value="device">device</option>
    </select>
  );

  const idInput = (
    <input
      className={inputCls}
      value={values.objectId}
      disabled={disabled}
      readOnly={idReadonly}
      onChange={e => onChange({ objectId: e.target.value })}
      placeholder="Например, s7"
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

  const topicInput = (
    <input
      className={variant === 'table' ? cn('table-input', { readonly: true }) : cn('input', { readonly: true })}
      value={topic}
      readOnly
    />
  );

  if (variant === 'table') {
    return (
      <>
        <table className={cn('table')}>
          <tbody>
            <tr>
              <th className={cn('table-label')}>Название</th>
              <td>{nameInput}</td>
            </tr>
            <tr>
              <th className={cn('table-label')}>Тип</th>
              <td>{typeSelect}</td>
            </tr>
            <tr>
              <th className={cn('table-label')}>ID объекта</th>
              <td>{idInput}</td>
            </tr>
            <tr>
              <th className={cn('table-label')}>Юнит</th>
              <td>{unitId}</td>
            </tr>
            <tr>
              <th className={cn('table-label')}>Описание</th>
              <td>{descInput}</td>
            </tr>
            <tr>
              <th className={cn('table-label')}>Топик</th>
              <td>{topicInput}</td>
            </tr>
          </tbody>
        </table>

        <table className={cn('table')}>
          <thead>
            <tr>
              <th>Ключ</th>
              <th>Модель</th>
              <th>Ед.</th>
              <th>Знаков</th>
              <th className={cn('table-remove-col')} />
            </tr>
          </thead>
          <tbody>
            {values.spec.map((row, index) => (
              <tr key={index}>
                <td>
                  <input
                    className={cn('table-input')}
                    value={row.key}
                    disabled={disabled}
                    onChange={e => onSpecChange(index, { key: e.target.value })}
                    placeholder="temperature"
                    title="Ключ значения — имя поля в JSON-пейлоаде"
                  />
                </td>
                <td>
                  <input
                    className={cn('table-input')}
                    value={row.model}
                    disabled={disabled}
                    onChange={e => onSpecChange(index, { model: e.target.value })}
                    placeholder="dht22"
                    title="Модель датчика/устройства (dht22, relay, led…)"
                  />
                </td>
                <td>
                  <input
                    className={cn('table-input')}
                    value={row.unit}
                    disabled={disabled}
                    onChange={e => onSpecChange(index, { unit: e.target.value })}
                    placeholder="°C, %"
                    title="Единица измерения — можно оставить пустым"
                  />
                </td>
                <td>
                  <input
                    className={cn('table-input')}
                    value={row.minorPart ?? ''}
                    disabled={disabled}
                    onChange={e => onSpecChange(index, { minorPart: e.target.value })}
                    placeholder="2"
                    title="Количество знаков после запятой — можно оставить пустым"
                    type="number"
                  />
                </td>
                <td className={cn('table-remove-col')}>
                  <button
                    type="button"
                    className={cn('table-remove')}
                    disabled={disabled}
                    onClick={() => onSpecRemove(index)}
                  >
                    ✕
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
        <button type="button" className={cn('table-add')} disabled={disabled} onClick={onSpecAdd}>
          + Добавить замер
        </button>
        <span className={cn('table-hint')}>
          Один объект может иметь несколько замеров — температура и влажность у одного DHT22.
          Ключ — имя значения, модель — сенсор/устройство.
        </span>
      </>
    );
  }

  return (
    <>
      <label className={cn('field')}>
        <span className={cn('label')}>Название</span>
        {nameInput}
      </label>

      <label className={cn('field')}>
        <span className={cn('label')}>ID объекта</span>
        {idInput}
      </label>

      <label className={cn('field')}>
        <span className={cn('label')}>Тип</span>
        {typeSelect}
      </label>

      <label className={cn('field')}>
        <span className={cn('label')}>Описание</span>
        {descInput}
      </label>

      <div className={cn('field')}>
        <span className={cn('label')}>Замеры (spec)</span>
        {values.spec.map((row, index) => (
          <div key={index} className={cn('spec-row')}>
            <label className={cn('spec-col')}>
              <span className={cn('spec-label')}>Ключ</span>
              <input
                className={cn('spec-input')}
                value={row.key}
                disabled={disabled}
                onChange={e => onSpecChange(index, { key: e.target.value })}
                placeholder="temperature"
                title="Ключ значения — имя поля в JSON-пейлоаде"
              />
            </label>
            <label className={cn('spec-col')}>
              <span className={cn('spec-label')}>Модель</span>
              <input
                className={cn('spec-input')}
                value={row.model}
                disabled={disabled}
                onChange={e => onSpecChange(index, { model: e.target.value })}
                placeholder="dht22"
                title="Модель датчика/устройства (dht22, relay, led…)"
              />
            </label>
            <label className={cn('spec-col')}>
              <span className={cn('spec-label')}>Ед.</span>
              <input
                className={cn('spec-input')}
                value={row.unit}
                disabled={disabled}
                onChange={e => onSpecChange(index, { unit: e.target.value })}
                placeholder="°C, %"
                title="Единица измерения — можно оставить пустым"
              />
            </label>
            <label className={cn('spec-col')}>
              <span className={cn('spec-label')}>Знаков</span>
              <input
                className={cn('spec-input')}
                value={row.minorPart ?? ''}
                disabled={disabled}
                onChange={e => onSpecChange(index, { minorPart: e.target.value })}
                placeholder="2"
                title="Количество знаков после запятой — можно оставить пустым"
                type="number"
              />
            </label>
            <button
              type="button"
              className={cn('spec-remove')}
              disabled={disabled}
              onClick={() => onSpecRemove(index)}
            >
              ✕
            </button>
          </div>
        ))}
        <button
          type="button"
          className={cn('spec-add')}
          disabled={disabled}
          onClick={onSpecAdd}
        >
          + Добавить замер
        </button>
        <span className={cn('spec-hint')}>
          Один объект может иметь несколько замеров — температура и влажность у одного
          DHT22. Ключ — имя значения, модель — сенсор/устройство.
        </span>
      </div>

      <label className={cn('field')}>
        <span className={cn('label')}>Топик</span>
        {topicInput}
      </label>
    </>
  );
};
