import { useEffect, useState } from 'react';
import { createPortal } from 'react-dom';
import { createCn } from 'bem-react-classname';

import { mockApi, isMockMode } from '../mock-api';
import { UnitFormFields, UnitFormValues, emptyUnitFormValues } from '../unit-form';

import './styles.css';

const cn = createCn('create-unit-modal');

interface CreateUnitModalProps {
  open: boolean;
  onClose: () => void;
  onCreated: () => void;
}

export const CreateUnitModal = ({ open, onClose, onCreated }: CreateUnitModalProps) => {
  const [values, setValues] = useState<UnitFormValues>(() => emptyUnitFormValues());
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    if (open) {
      setValues(emptyUnitFormValues());
      setError(null);
      setSubmitting(false);
    }
  }, [open]);

  if (!open) {
    return null;
  }

  const handleSubmit = async () => {
    if (!values.unitId.trim() || !values.name.trim()) {
      setError('Заполните ID и название юнита');
      return;
    }

    const payload = {
      id: values.unitId.trim(),
      name: values.name.trim(),
      ...(values.description.trim() ? { description: values.description.trim() } : {}),
    };

    setSubmitting(true);
    setError(null);
    try {
      if (isMockMode()) {
        await mockApi.createUnit(payload);
      } else {
        const response = await fetch('/api/v1/units', {
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
      setError(err instanceof Error ? err.message : 'Не удалось создать юнит');
    } finally {
      setSubmitting(false);
    }
  };

  return createPortal(
    <div className={cn()} onClick={onClose}>
      <div className={cn('dialog')} onClick={e => e.stopPropagation()}>
        <div className={cn('title')}>Новый юнит</div>

        <UnitFormFields
          values={values}
          onChange={patch => setValues(prev => ({ ...prev, ...patch }))}
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
