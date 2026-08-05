import { useEffect, useState } from 'react';
import { createPortal } from 'react-dom';
import { createCn } from 'bem-react-classname';

import { mockApi, isMockMode } from '../mock-api';

import './styles.css';

const cn = createCn('create-object-modal');

type ObjectType = 'sensor' | 'device';

interface SpecRow {
  key: string;
  model: string;
  unit: string;
  minorPart?: string;
}

interface CreateObjectModalProps {
  open: boolean;
  unitId: string;
  defaultType: ObjectType;
  onClose: () => void;
  onCreated: () => void;
}

const emptySpecRow = (): SpecRow => ({ key: '', model: '', unit: '', minorPart: '' });

export const CreateObjectModal = ({
  open,
  unitId,
  defaultType,
  onClose,
  onCreated,
}: CreateObjectModalProps) => {
  const [name, setName] = useState('');
  const [objectId, setObjectId] = useState('');
  const [type, setType] = useState<ObjectType>(defaultType);
  const [description, setDescription] = useState('');
  const [spec, setSpec] = useState<SpecRow[]>([emptySpecRow()]);
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (open) {
      setName('');
      setObjectId('');
      setType(defaultType);
      setDescription('');
      setSpec([emptySpecRow()]);
      setError(null);
      setSubmitting(false);
    }
  }, [open, defaultType]);

  if (!open) {
    return null;
  }

  const topic = `${type}/${unitId}/${objectId}`;

  const updateSpecRow = (index: number, patch: Partial<SpecRow>) => {
    setSpec(prev => prev.map((row, i) => (i === index ? { ...row, ...patch } : row)));
  };

  const addSpecRow = () => setSpec(prev => [...prev, emptySpecRow()]);

  const removeSpecRow = (index: number) => {
    setSpec(prev => (prev.length === 1 ? prev : prev.filter((_, i) => i !== index)));
  };

  const handleSubmit = async () => {
    if (!name.trim() || !objectId.trim()) {
      setError('Заполните название и ID объекта');
      return;
    }
    if (spec.length === 0 || spec.some(s => !s.key.trim() || !s.model.trim())) {
      setError('Заполните key и model для spec-записей');
      return;
    }

    const payload = {
      id: objectId.trim(),
      name: name.trim(),
      type,
      spec: spec.map(s => ({
        key: s.key.trim(),
        model: s.model.trim(),
        ...(s.unit.trim() ? { unit: s.unit.trim() } : {}),
        ...(s.minorPart !== undefined && s.minorPart !== ''
          ? { minorPart: Number(s.minorPart) }
          : {}),
      })),
      ...(description.trim() ? { description: description.trim() } : {}),
      unitId,
    };

    setSubmitting(true);
    setError(null);
    try {
      if (isMockMode()) {
        await mockApi.addObject(payload);
      } else {
        const response = await fetch('/api/v1/objects', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
        });
        if (!response.ok) {
          const data = await response.json().catch(() => null);
          throw new Error(data?.error || `HTTP error! status: ${response.status}`);
        }
      }
      onCreated();
      onClose();
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось создать объект');
    } finally {
      setSubmitting(false);
    }
  };

  return createPortal(
    <div className={cn()} onClick={onClose}>
      <div className={cn('dialog')} onClick={e => e.stopPropagation()}>
        <div className={cn('title')}>Новый объект</div>
        <div className={cn('unit-hint')}>Юнит: {unitId}</div>

        <label className={cn('field')}>
          <span className={cn('label')}>Название</span>
          <input
            className={cn('input')}
            value={name}
            onChange={e => setName(e.target.value)}
            placeholder="Например, DHT22"
          />
        </label>

        <label className={cn('field')}>
          <span className={cn('label')}>ID объекта</span>
          <input
            className={cn('input')}
            value={objectId}
            onChange={e => setObjectId(e.target.value)}
            placeholder="Например, s7"
          />
        </label>

        <label className={cn('field')}>
          <span className={cn('label')}>Тип</span>
          <select
            className={cn('input')}
            value={type}
            onChange={e => setType(e.target.value as ObjectType)}
          >
            <option value="sensor">sensor</option>
            <option value="device">device</option>
          </select>
        </label>

        <label className={cn('field')}>
          <span className={cn('label')}>Описание</span>
          <input
            className={cn('input')}
            value={description}
            onChange={e => setDescription(e.target.value)}
            placeholder="Необязательно"
          />
        </label>

        <div className={cn('field')}>
          <span className={cn('label')}>Замеры (spec)</span>
          {spec.map((row, index) => (
            <div key={index} className={cn('spec-row')}>
              <label className={cn('spec-col')}>
                <span className={cn('spec-label')}>Ключ</span>
                <input
                  className={cn('spec-input')}
                  value={row.key}
                  onChange={e => updateSpecRow(index, { key: e.target.value })}
                  placeholder="temperature"
                  title="Ключ значения — имя поля в JSON-пейлоаде"
                />
              </label>
              <label className={cn('spec-col')}>
                <span className={cn('spec-label')}>Модель</span>
                <input
                  className={cn('spec-input')}
                  value={row.model}
                  onChange={e => updateSpecRow(index, { model: e.target.value })}
                  placeholder="dht22"
                  title="Модель датчика/устройства (dht22, relay, led…)"
                />
              </label>
              <label className={cn('spec-col')}>
                <span className={cn('spec-label')}>Ед.</span>
                <input
                  className={cn('spec-input')}
                  value={row.unit}
                  onChange={e => updateSpecRow(index, { unit: e.target.value })}
                  placeholder="°C, %"
                  title="Единица измерения — можно оставить пустым"
                />
              </label>
              <label className={cn('spec-col')}>
                <span className={cn('spec-label')}>Знаков</span>
                <input
                  className={cn('spec-input')}
                  value={row.minorPart ?? ''}
                  onChange={e => updateSpecRow(index, { minorPart: e.target.value })}
                  placeholder="2"
                  title="Количество знаков после запятой — можно оставить пустым"
                  type="number"
                />
              </label>
              <button
                type="button"
                className={cn('spec-remove')}
                onClick={() => removeSpecRow(index)}
              >
                ✕
              </button>
            </div>
          ))}
          <button type="button" className={cn('spec-add')} onClick={addSpecRow}>
            + Добавить замер
          </button>
          <span className={cn('spec-hint')}>
            Один объект может иметь несколько замеров — температура и влажность у одного
            DHT22. Ключ — имя значения, модель — сенсор/устройство.
          </span>
        </div>

        <div className={cn('field')}>
          <span className={cn('label')}>Топик</span>
          <input className={cn('input', { readonly: true })} value={topic} readOnly />
        </div>

        {error && <div className={cn('error')}>{error}</div>}

        <div className={cn('actions')}>
          <button type="button" className={cn('button')} onClick={onClose}>
            Отмена
          </button>
          <button
            type="button"
            className={cn('button', { primary: true })}
            onClick={handleSubmit}
            disabled={submitting}
          >
            {submitting ? 'Создание…' : 'Создать'}
          </button>
        </div>
      </div>
    </div>,
    document.body,
  );
};
