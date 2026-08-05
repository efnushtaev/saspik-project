import { useEffect, useState } from 'react';
import { createPortal } from 'react-dom';
import { createCn } from 'bem-react-classname';

import { mockApi, isMockMode } from '../mock-api';
import {
  ObjectFormFields,
  ObjectFormValues,
  ObjectType,
  emptyObjectFormValues,
  emptySpecRow,
} from '../object-form';

import './styles.css';

const cn = createCn('create-object-modal');

interface CreateObjectModalProps {
  open: boolean;
  unitId: string;
  defaultType: ObjectType;
  onClose: () => void;
  onCreated: () => void;
}

export const CreateObjectModal = ({
  open,
  unitId,
  defaultType,
  onClose,
  onCreated,
}: CreateObjectModalProps) => {
  const [values, setValues] = useState<ObjectFormValues>(() =>
    emptyObjectFormValues(defaultType),
  );
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (open) {
      setValues(emptyObjectFormValues(defaultType));
      setError(null);
      setSubmitting(false);
    }
  }, [open, defaultType]);

  if (!open) {
    return null;
  }

  const updateSpecRow = (index: number, patch: Partial<ObjectFormValues['spec'][number]>) => {
    setValues(prev => ({
      ...prev,
      spec: prev.spec.map((row, i) => (i === index ? { ...row, ...patch } : row)),
    }));
  };

  const addSpecRow = () =>
    setValues(prev => ({ ...prev, spec: [...prev.spec, emptySpecRow()] }));

  const removeSpecRow = (index: number) =>
    setValues(prev => ({
      ...prev,
      spec: prev.spec.length === 1 ? prev.spec : prev.spec.filter((_, i) => i !== index),
    }));

  const handleSubmit = async () => {
    if (!values.name.trim() || !values.objectId.trim()) {
      setError('Заполните название и ID объекта');
      return;
    }
    if (values.spec.length === 0 || values.spec.some(s => !s.key.trim() || !s.model.trim())) {
      setError('Заполните key и model для spec-записей');
      return;
    }

    const payload = {
      id: values.objectId.trim(),
      name: values.name.trim(),
      type: values.type,
      spec: values.spec.map(s => ({
        key: s.key.trim(),
        model: s.model.trim(),
        ...(s.unit.trim() ? { unit: s.unit.trim() } : {}),
        ...(s.minorPart !== undefined && s.minorPart !== ''
          ? { minorPart: Number(s.minorPart) }
          : {}),
      })),
      ...(values.description.trim() ? { description: values.description.trim() } : {}),
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

        <ObjectFormFields
          values={values}
          unitId={unitId}
          onChange={patch => setValues(prev => ({ ...prev, ...patch }))}
          onSpecChange={updateSpecRow}
          onSpecAdd={addSpecRow}
          onSpecRemove={removeSpecRow}
        />

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
