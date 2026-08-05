import { ObjectsDto } from "../../dto/objects.dto";

export interface IObjectsService {
  getObjects(typeFilter?: string, unitId?: string): Promise<ObjectsDto[]>;

  getByIds(ids: string[], typeFilter?: string, unitId?: string): Promise<ObjectsDto[]>;

  callCommand(deviceId: string, value: string, unitId?: string): Promise<void>;

  getLastSensorsData(ids: string[]): Promise<Record<string, unknown>>;

  getObjectState(topic: string, field?: string): Promise<number | string | boolean | null>;

  createObject(dto: Omit<ObjectsDto, "topic">, unitId: string): Promise<ObjectsDto>;
}
