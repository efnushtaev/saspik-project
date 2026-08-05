import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IObjectsService } from ".";
import { TYPES } from "../../types";
import { TEMPORARY_ANY } from "../../types";
import { IStateStoreService } from "../state-store/stateStore.interface";
import { IMqttService } from "../mqtt";
import { ObjectsType, ObjectsDto } from "../../dto/objects.dto";
import { ObjectEntity, ObjectsRepository, toObjectsDto } from "../data-store";

@injectable()
export class ObjectsService implements IObjectsService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.StateStoreService) private stateStore: IStateStoreService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
    @inject(TYPES.ObjectsRepository) private objectsRepository: ObjectsRepository,
  ) {
    this.logger.log("[ObjectsService] initialized");
  }

  async getObjects(typeFilter?: string, unitId?: string): Promise<TEMPORARY_ANY[]> {
    this.logger.log(
      `[ObjectsService] getObjects${typeFilter ? ` filter=${typeFilter}` : ""}${unitId ? ` unit=${unitId}` : ""}`,
    );

    let result = await this.objectsRepository.findByUnitId(unitId);

    if (typeFilter) {
      result = result.filter((obj) => obj.type === typeFilter);
    }

    return result.map(toObjectsDto);
  }

  async getByIds(ids: string[], typeFilter?: string, unitId?: string): Promise<TEMPORARY_ANY[]> {
    this.logger.log(`[ObjectsService] getByIds`);

    let result = await this.objectsRepository.findByIds(ids);

    if (typeFilter) {
      result = result.filter((obj) => obj.type === typeFilter);
    }

    if (unitId) {
      result = result.filter((obj) => obj.unitId === unitId);
    }

    return result.map(toObjectsDto);
  }

  async callCommand(deviceId: string, value: string, unitId?: string): Promise<void> {
    this.logger.log(
      `[ObjectsService] callCommand for device ${deviceId}, value ${value}${unitId ? ` unit=${unitId}` : ""}`,
    );

    const device = (await this.objectsRepository.findByUnitId(unitId)).find(
      (obj) => obj.id === deviceId && obj.type === ObjectsType.DEVICE,
    );
    if (device) {
      const specKey = device.spec?.[0]?.key || "state";
      const payload = JSON.stringify({ [specKey]: value });
      await this.mqttService.publish(device.topic, payload);
    }
  }

  async getLastSensorsData(ids: string[]): Promise<Record<string, unknown>> {
    this.logger.log(`[ObjectsService] getLastSensorsData`);

    return {};
  }

  async getObjectState(topic: string, field?: string): Promise<number | string | boolean | null> {
    this.logger.log(`[ObjectsService] getObjectState topic=${topic} field=${field}`);

    const stored = await this.stateStore.get(topic, field);
    return stored?.value ?? null;
  }

  async createObject(dto: Omit<ObjectsDto, "topic">, unitId: string): Promise<ObjectsDto> {
    this.logger.log(`[ObjectsService] createObject id=${dto.id} unit=${unitId}`);

    if (!unitId) {
      throw new Error("unitId is required");
    }
    if (!dto.id || !dto.name || !dto.type) {
      throw new Error("Object must have id, name and type");
    }
    if (!Array.isArray(dto.spec) || dto.spec.length === 0) {
      throw new Error("Object must have at least one spec entry");
    }
    for (const spec of dto.spec) {
      if (!spec.key || !spec.model) {
        throw new Error("Each spec entry must have key and model");
      }
    }

    const topic = `${dto.type}/${unitId}/${dto.id}`;
    const entity: ObjectEntity = {
      id: dto.id,
      name: dto.name,
      type: dto.type,
      spec: dto.spec,
      description: dto.description,
      topic,
      unitId,
    };
    const saved = await this.objectsRepository.create(entity);
    return toObjectsDto(saved);
  }
}
