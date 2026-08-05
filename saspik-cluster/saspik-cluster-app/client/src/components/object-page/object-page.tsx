import { useCallback, useEffect, useState } from 'react';
import { useLocation, useNavigate, useParams } from 'react-router-dom';
import { createCn } from 'bem-react-classname';

import { mockApi, isMockMode } from '../mock-api';
import { ObjectFormFields, ObjectFormValues, emptyObjectFormValues, emptySpecRow } from '../object-form';
import { ConfirmModal } from '../confirm-modal';
import { ObjectItem } from '../objects-list/types';
import { NAVIGATION_PATHS } from '../constants';
import { BasePage } from '../pages-routes';

import './styles.css';

const cn = createCn('object-page');

const toFormValues = (obj: ObjectItem): ObjectFormValues => ({
  name: obj.name,
  objectId: obj.id,
  type: obj.type,
  description: obj.description || '',
  spec: (obj.spec || []).map(s => ({
    key: s.key,
    model: s.spec.model,
    unit: s.spec.unit || '',
    minorPart: s.spec.minorPart !== undefined ? String(s.spec.minorPart) : '',
  })),
});

export const ObjectPage = () => {
  const { objectId = '' } = useParams();
  const navigate = useNavigate();
  const location = useLocation();
  const unitId = new URLSearchParams(location.search).get('id') || '';

  const [object, setObject] = useState<ObjectItem | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const [editing, setEditing] = useState(false);
  const [values, setValues] = useState<ObjectFormValues>(() => emptyObjectFormValues('sensor'));
  const [saveError, setSaveError] = useState<string | null>(null);
  const [saving, setSaving] = useState(false);

  const [confirmOpen, setConfirmOpen] = useState(false);
  const [deleting, setDeleting] = useState(false);
  const [deleteError, setDeleteError] = useState<string | null>(null);

  const fetchObject = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      let fetched: ObjectItem | null = null;
      if (isMockMode()) {
        const res = await mockApi.getObject(objectId);
        fetched = res.object;
      } else {
        const response = await fetch('/api/v1/objects/getByIds', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ id: [objectId], unitId }),
        });
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        fetched = data.objects?.[0] ?? null;
      }
      if (!fetched) {
        throw new Error('Объект не найден');
      }
      setObject(fetched);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Не удалось загрузить объект');
    } finally {
      setLoading(false);
    }
  }, [objectId, unitId]);

  useEffect(() => {
    setEditing(false);
    fetchObject();
  }, [fetchObject]);

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

  const startEdit = () => {
    if (!object) return;
    setValues(toFormValues(object));
    setSaveError(null);
    setEditing(true);
  };

  const handleSave = async () => {
    if (!object) return;
    if (!values.name.trim()) {
      setSaveError('Заполните название объекта');
      return;
    }
    if (values.spec.length === 0 || values.spec.some(s => !s.key.trim() || !s.model.trim())) {
      setSaveError('Заполните key и model для spec-записей');
      return;
    }

    const payload = {
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

    setSaving(true);
    setSaveError(null);
    try {
      if (isMockMode()) {
        await mockApi.updateObject(object.id, payload);
      } else {
        const response = await fetch(`/api/v1/objects/${object.id}`, {
          method: 'PATCH',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload),
        });
        if (!response.ok) {
          const data = await response.json().catch(() => null);
          throw new Error(data?.error || `HTTP error! status: ${response.status}`);
        }
      }
      window.dispatchEvent(new CustomEvent('objects-updated'));
      setEditing(false);
      await fetchObject();
    } catch (err) {
      setSaveError(err instanceof Error ? err.message : 'Не удалось сохранить объект');
    } finally {
      setSaving(false);
    }
  };

  const handleDelete = async () => {
    if (!object) return;
    setDeleting(true);
    setDeleteError(null);
    try {
      if (isMockMode()) {
        await mockApi.deleteObject(object.id);
      } else {
        const response = await fetch(`/api/v1/objects/${object.id}`, {
          method: 'DELETE',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ unitId }),
        });
        if (!response.ok) {
          const data = await response.json().catch(() => null);
          throw new Error(data?.error || `HTTP error! status: ${response.status}`);
        }
      }
      window.dispatchEvent(new CustomEvent('objects-updated'));
      navigate(`${NAVIGATION_PATHS[object.type]}?id=${unitId}`);
    } catch (err) {
      setDeleteError(err instanceof Error ? err.message : 'Не удалось удалить объект');
    } finally {
      setDeleting(false);
    }
  };

  const handleBack = () => {
    if (object) {
      navigate(`${NAVIGATION_PATHS[object.type]}?id=${unitId}`);
    } else {
      navigate(-1);
    }
  };

  if (loading) {
    return (
      <BasePage>
        <div className={'rotate-scale-up'} />
      </BasePage>
    );
  }

  if (error || !object) {
    return (
      <BasePage>
        <div className={cn()}>
          <div className={cn('error')}>{error || 'Объект не найден'}</div>
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
        <div className={cn('header')}>
          <div className={cn('title')}>{object.name}</div>
          <div className={cn('actions')}>
            {!editing ? (
              <>
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
              </>
            ) : (
              <>
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
              </>
            )}
          </div>
        </div>

        {object.description && <div className={cn('description')}>{object.description}</div>}

        <table className={cn('spec')}>
          <tbody>
            <tr>
              <th className={cn('spec-label')}>Тип</th>
              <td>{object.type}</td>
            </tr>
            <tr>
              <th className={cn('spec-label')}>ID</th>
              <td>{object.id}</td>
            </tr>
            <tr>
              <th className={cn('spec-label')}>Юнит</th>
              <td>{unitId}</td>
            </tr>
            <tr>
              <th className={cn('spec-label')}>Топик</th>
              <td>{object.topic || `${object.type}/${unitId}/${object.id}`}</td>
            </tr>
          </tbody>
        </table>

        {editing ? (
          <>
            <ObjectFormFields
              values={values}
              unitId={unitId}
              idReadonly
              variant="table"
              onChange={patch => setValues(prev => ({ ...prev, ...patch }))}
              onSpecChange={updateSpecRow}
              onSpecAdd={addSpecRow}
              onSpecRemove={removeSpecRow}
            />
            {saveError && <div className={cn('error')}>{saveError}</div>}
          </>
        ) : (
          <table className={cn('spec')}>
            <thead>
              <tr>
                <th>Ключ</th>
                <th>Значение</th>
                <th>Модель</th>
                <th>Ед.</th>
                <th>Знаков</th>
              </tr>
            </thead>
            <tbody>
              {object.spec.map((s, i) => (
                <tr key={i}>
                  <td>{s.key}</td>
                  <td className={cn('spec-value')}>{s.value ?? '--'}</td>
                  <td>{s.spec.model}</td>
                  <td>{s.spec.unit || ''}</td>
                  <td>{s.spec.minorPart ?? ''}</td>
                </tr>
              ))}
            </tbody>
          </table>
        )}

        {deleteError && <div className={cn('error')}>{deleteError}</div>}

        <ConfirmModal
          open={confirmOpen}
          title="Удалить объект?"
          message={`Объект «${object.name}» (${object.id}) будет удалён безвозвратно.`}
          confirmLabel="Удалить"
          danger
          submitting={deleting}
          onConfirm={handleDelete}
          onClose={() => setConfirmOpen(false)}
        />
      </div>
    </BasePage>
  );
};
