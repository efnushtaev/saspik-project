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
