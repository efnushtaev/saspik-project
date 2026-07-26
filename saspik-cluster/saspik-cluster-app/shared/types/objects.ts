export interface ObjectsListItem {
  id: string;
  name: string;
  type: 'sensor' | 'device';
  spec: {
    model: string;
    unit?: string;
    sensorType?: 'humidity' | 'temperature' | 'float';
  };
  description?: string;
  topic: string;
  value: number | string | boolean | null;
}

export interface ListResponse {
  objects: ObjectsListItem[];
}
