export type ObjectType = 'sensor' | 'device';

export interface SpecRow {
  key: string;
  model: string;
  unit: string;
  minorPart?: string;
}

export interface ObjectFormValues {
  name: string;
  objectId: string;
  type: ObjectType;
  description: string;
  spec: SpecRow[];
}

export const emptySpecRow = (): SpecRow => ({ key: '', model: '', unit: '', minorPart: '' });

export const emptyObjectFormValues = (type: ObjectType): ObjectFormValues => ({
  name: '',
  objectId: '',
  type,
  description: '',
  spec: [emptySpecRow()],
});

// Постфиксы функциональных ролей (sensor — мониторинг, device — автоматика).
// В сегмент топика идёт только база objectId без постфикса.
export const stripIdPostfix = (id: string): string => {
  for (const postfix of ['sensor', 'device'] as ObjectType[]) {
    if (id.endsWith(`-${postfix}`)) {
      return id.slice(0, -`-${postfix}`.length);
    }
  }
  return id;
};

export const withTypePostfix = (id: string, type: ObjectType): string =>
  `${stripIdPostfix(id.trim())}-${type}`;
