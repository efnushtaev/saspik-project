export type ObjectItemType = 'sensor' | 'device';

export interface ObjectItem {
  id: string;
  name: string;
  type: ObjectItemType;
  spec: {
    key: string;
    value?: string | number | boolean | null;
    spec: {
      model: string;
      unit?: string;
      minorPart?: number;
    };
  }[];
  description?: string;
  topic?: string;
}

export type PageObjectType = 'sensor' | 'device';

export interface ObjectsListProps {
  type?: PageObjectType;
  unitId?: string;
}
