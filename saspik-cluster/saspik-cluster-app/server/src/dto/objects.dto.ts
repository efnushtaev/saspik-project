export enum ObjectsType {
  SENSOR = "sensor",
  DEVICE = "device",
}

export enum SensorObjectsType {
  HUMIDITY = "humidity",
  TEMPERATURE = "temperature",
  FLOAT_SENSOR = "float",
}

export class ObjectsDtoSpec {
  key: string;
  model: string;
  unit: string;
  minorPart?: number;
}

export class ObjectsDto {
  id: string;
  name: string;
  type: ObjectsType;
  spec: ObjectsDtoSpec[];
  description?: string;
  topic: string;
}
