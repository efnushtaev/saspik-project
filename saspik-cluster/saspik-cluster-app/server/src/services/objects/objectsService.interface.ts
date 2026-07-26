import { ObjectsDto } from "../../dto/objects.dto";

export interface IObjectsService {
  getObjects(typeFilter?: string): Promise<ObjectsDto[]>;

  getByIds(ids: string[], typeFilter?: string): Promise<ObjectsDto[]>;

  callCommand(deviceId: string, value: string): Promise<void>;

  getLastSensorsData(ids: string[]): Promise<Record<string, unknown>>;

  getObjectState(id: string, field?: string): Promise<number | string | boolean | null>;
}
