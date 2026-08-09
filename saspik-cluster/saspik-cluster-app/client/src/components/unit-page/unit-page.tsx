import { useCallback, useEffect, useState } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { mockApi, isMockMode } from '../mock-api';
import { UnitFormFields, UnitFormValues, emptyUnitFormValues } from '../unit-form';
import { ConfirmModal } from '../confirm-modal';
import { CreateObjectModal } from '../create-object-modal';
import { ObjectsList } from '../objects-list';
import { BasePage } from '../pages-routes';
import { usePageHeader } from '../top-bar/page-header-context';

import './styles.css';

const cn = createCn('unit-page');

interface UnitObject {
  id: string;
  name: string;
}

interface Unit {
  id: string;
  name: string;
  description?: string;
  objects: UnitObject[];
  rules: unknown[];
}

const toFormValues = (unit: Unit): UnitFormValues => ({
  unitId: unit.id,
  name: unit.name,
  description: unit.description || '',
});

export const UnitPage = () => {
  const { unitId = '' } = useParams();
  const navigate = useNavigate();

  const [unit, setUnit] = useState<Unit | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [editing, setEditing] = useState(false);
  const [values, setValues] = useState<UnitFormValues>(() => emptyUnitFormValues());
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);

  const [confirmOpen, setConfirmOpen] = useState(false);
  const [deleting, setDeleting] = useState(false);
  const [deleteError, setDeleteError] = useState<string | null>(null);

  const [isCreateObjectOpen, setIsCreateObjectOpen] = useState(false);

  const fetchUnit = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      let fetched: Unit | null = null;
      if (isMockMode()) {
        const res = await mockApi.getUnit(unitId);
        fetched = res.unit;
      } else {
        const response = await fetch(`/api/v1/units/${unitId}`);
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        fetched = data.unit ?? null;
      }
      if (!fetched) {
        throw new Error('Юнит не найден');
      }
      setUnit(fetched);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить юнит');
    } finally {
      setLoading(false);
    }
  }, [unitId]);

  useEffect(() => {
    setEditing(false);
    fetchUnit();
  }, [fetchUnit]);

  const startEdit = () => {
    if (!unit) return;
    setValues(toFormValues(unit));
    setSaveError(null);
    setEditing(true);
  };

  const handleSave = async () => {
    if (!unit) return;
    if (!values.name.trim()) {
      setSaveError('Заполните название юнита');
      return;
    }

    const payload = {
      name: values.name.trim(),
      ...(values.description.trim() ? { description: values.description.trim() } : {}),
    };

    setSaving(true);
    setSaveError(null);
    try {
      if (isMockMode()) {
        await mockApi.updateUnit(unit.id, payload);
      } else {
        const response = await fetch(`/api/v1/units/${unit.id}`, {
          method: 'PATCH',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
        });
        if (!response.ok) {
          const data = await response.json().catch(() => null);
          throw new Error(data?.error || `HTTP error! status: ${response.status}`);
        }
      }
      window.dispatchEvent(new CustomEvent('units-updated'));
      setEditing(false);
      await fetchUnit();
    } catch (err) {
      setSaveError(err instanceof Error ? err.message : 'Не удалось сохранить юнит');
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async () => {
    if (!unit) return;
    setDeleting(true);
    setDeleteError(null);
    try {
      if (isMockMode()) {
        await mockApi.deleteUnit(unit.id);
      } else {
        const response = await fetch(`/api/v1/units/${unit.id}`, {
          method: 'DELETE',
          headers: { 'Content-Type': 'application/json' },
        });
        if (!response.ok) {
          const data = await response.json().catch(() => null);
          throw new Error(data?.error || `HTTP error! status: ${response.status}`);
        }
      }
      window.dispatchEvent(new CustomEvent('units-updated'));
      navigate('/');
    } catch (err) {
      setDeleteError(err instanceof Error ? err.message : 'Не удалось удалить юнит');
    } finally {
      setDeleting(false);
    }
  };

  const handleBack = () => {
    navigate('/');
  };

  usePageHeader(unit && !error ? unit.name : null);

  if (loading) {
    return (
      <BasePage>
        <div className={'rotate-scale-up'} />
      </BasePage>
    );
  }

  if (error || !unit) {
    return (
      <BasePage>
        <div className={cn()}>
          <div className={cn('error')}>{error || 'Юнит не найден'}</div>
          <button type="button" className={cn('button')} onClick={handleBack}>
            Назад
          </button>
        </div>
      </BasePage>
    );
  }

  return (
    <BasePage>
      <div className={cn()}>
        <div className={cn('section')}>
          <div className={cn('section-title')}>Информация</div>

          {editing ? (
            <div className={cn('header')}>
              <div className={cn('actions')}>
                <button type="button" className={cn('button')} onClick={() => setEditing(false)}>
                  Отмена
                </button>
                <button
                  type="button"
                  className={cn('button', { primary: true })}
                  onClick={handleSave}
                  disabled={saving}
                >
                  {saving ? 'Сохранение…' : 'Сохранить'}
                </button>
              </div>
            </div>
          ) : (
            <div className={cn('header')}>
              <div className={cn('actions')}>
                <button type="button" className={cn('button')} onClick={handleBack}>
                  Назад
                </button>
                <button
                  type="button"
                  className={cn('button', { primary: true })}
                  onClick={startEdit}
                >
                  Редактировать
                </button>
                <button
                  type="button"
                  className={cn('button', { danger: true })}
                  onClick={() => setConfirmOpen(true)}
                >
                  Удалить
                </button>
              </div>
            </div>
          )}

          {unit.description && <div className={cn('description')}>{unit.description}</div>}

          {editing ? (
            <>
              <UnitFormFields
                values={values}
                idReadonly
                variant="table"
                onChange={patch => setValues(prev => ({ ...prev, ...patch }))}
              />
              {saveError && <div className={cn('error')}>{saveError}</div>}
            </>
          ) : (
            <table className={cn('spec')}>
              <tbody>
                <tr>
                  <th className={cn('spec-label')}>ID</th>
                  <td>{unit.id}</td>
                </tr>
                <tr>
                  <th className={cn('spec-label')}>Название</th>
                  <td>{unit.name}</td>
                </tr>
                <tr>
                  <th className={cn('spec-label')}>Описание</th>
                  <td>{unit.description || '—'}</td>
                </tr>
                <tr>
                  <th className={cn('spec-label')}>Объектов</th>
                  <td>{unit.objects.length}</td>
                </tr>
              </tbody>
            </table>
          )}
        </div>

        <button
          type="button"
          className={cn('add-object')}
          onClick={() => setIsCreateObjectOpen(true)}
        >
          ＋ Добавить объект
        </button>

        <ObjectsList type="sensor" unitId={unit.id} />

        {deleteError && <div className={cn('error')}>{deleteError}</div>}

        <ConfirmModal
          open={confirmOpen}
          title="Удалить юнит?"
          message={`Юнит «${unit.name}» (${unit.id}) будет удалён вместе со всеми его объектами. Действие необратимо.`}
          confirmLabel="Удалить"
          danger
          submitting={deleting}
          onConfirm={handleDelete}
          onClose={() => setConfirmOpen(false)}
        />

        <CreateObjectModal
          open={isCreateObjectOpen}
          unitId={unit.id}
          defaultType="sensor"
          onClose={() => setIsCreateObjectOpen(false)}
          onCreated={() => window.dispatchEvent(new CustomEvent('objects-updated'))}
        />
      </div>
    </BasePage>
  );
};
