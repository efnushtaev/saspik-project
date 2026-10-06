import { ObjectsDto } from "../../dto/objects.dto";
import { StoredState } from "../state-store/stateStore.interface";

export type DeviceStatus = "online" | "offline";

export interface IObjectsService {
  getObjects(typeFilter?: string, unitId?: string): Promise<ObjectsDto[]>;

  getByIds(ids: string[], typeFilter?: string, unitId?: string): Promise<ObjectsDto[]>;

  callCommand(deviceId: string, value: string, unitId?: string): Promise<void>;

  getLastSensorsData(ids: string[]): Promise<Record<string, unknown>>;

  getObjectState(topic: string, field?: string): Promise<number | string | boolean | null>;

  getObjectStateEntry(topic: string, field?: string): Promise<StoredState | null>;

  getStatus(objectTopic: string): DeviceStatus | undefined;

  createObject(dto: Omit<ObjectsDto, "topic">, unitId: string): Promise<ObjectsDto>;

  updateObject(id: string, unitId: string, dto: Omit<ObjectsDto, "topic">): Promise<ObjectsDto | null>;

  deleteObject(id: string, unitId: string): Promise<boolean>;
}
