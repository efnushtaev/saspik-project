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
  /** ISO-время последнего сообщения по телеметрии */
  updatedAt?: string;
  /** Retained-статус устройства с топика device/…/status */
  status?: 'online' | 'offline';
}

export type PageObjectType = 'sensor' | 'device';

export interface ObjectsListProps {
  type?: PageObjectType;
  unitId?: string;
}
