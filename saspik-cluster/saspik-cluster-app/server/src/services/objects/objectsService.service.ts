import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { DeviceStatus, IObjectsService } from ".";
import { TYPES } from "../../types";
import { TEMPORARY_ANY } from "../../types";
import { IStateStoreService, StoredState } from "../state-store/stateStore.interface";
import { IMqttService } from "../mqtt";
import { ObjectsType, ObjectsDto } from "../../dto/objects.dto";
import { ObjectEntity, ObjectsRepository, toObjectsDto } from "../data-store";

// Retained-статусы устройств по ключу `${unitId}/${objectId}`
const STATUS_TOPIC_PATTERN = "device/+/+/status";
const STATUS_SUBSCRIBE_RETRY_MS = 3000;

@injectable()
export class ObjectsService implements IObjectsService {
  private deviceStatuses = new Map<string, DeviceStatus>();

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.StateStoreService) private stateStore: IStateStoreService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
    @inject(TYPES.ObjectsRepository) private objectsRepository: ObjectsRepository,
  ) {
    this.logger.log("[ObjectsService] initialized");
    this.subscribeDeviceStatus();
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
    const stored = await this.getObjectStateEntry(topic, field);
    return stored?.value ?? null;
  }

  async getObjectStateEntry(topic: string, field?: string): Promise<StoredState | null> {
    this.logger.log(`[ObjectsService] getObjectStateEntry topic=${topic} field=${field}`);
    return this.stateStore.get(topic, field);
  }

  getStatus(objectTopic: string): DeviceStatus | undefined {
    const [, unitId, objectId] = objectTopic.split("/");
    if (!unitId || !objectId) return undefined;
    return this.deviceStatuses.get(`${unitId}/${objectId}`);
  }

  /**
   * Подписка на retained-статусы устройств (device/+/+/status).
   * При старте MQTT может быть ещё не подключён — повторяем до успеха;
   * после успеха LocalMqttService сам переподписывается при реконнекте,
   * а retained-сообщения приходят сразу при подписке.
   */
  private subscribeDeviceStatus(): void {
    this.mqttService
      .subscribe(STATUS_TOPIC_PATTERN, (topic, message) => {
        this.handleStatusMessage(topic, message);
      })
      .then(() => {
        this.logger.log(`[ObjectsService] subscribed to ${STATUS_TOPIC_PATTERN}`);
      })
      .catch((err: unknown) => {
        this.logger.error(
          `[ObjectsService] status subscribe failed, retry in ${STATUS_SUBSCRIBE_RETRY_MS}ms:`,
          err,
        );
        setTimeout(() => this.subscribeDeviceStatus(), STATUS_SUBSCRIBE_RETRY_MS);
      });
  }

  private handleStatusMessage(topic: string, message: Buffer): void {
    try {
      const parsed = JSON.parse(message.toString()) as { status?: unknown };
      const status = parsed.status;
      if (status !== "online" && status !== "offline") return;
      const [, unitId, objectId] = topic.split("/");
      if (!unitId || !objectId) return;
      this.deviceStatuses.set(`${unitId}/${objectId}`, status);
      this.logger.log(`[ObjectsService] status ${unitId}/${objectId} -> ${status}`);
    } catch (err) {
      this.logger.error(`[ObjectsService] invalid status payload on ${topic}:`, err);
    }
  }

  async createObject(dto: Omit<ObjectsDto, "topic">, unitId: string): Promise<ObjectsDto> {
    this.logger.log(`[ObjectsService] createObject id=${dto.id} unit=${unitId}`);

    this.validateObjectDto(dto, unitId);

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

  async updateObject(
    id: string,
    unitId: string,
    dto: Omit<ObjectsDto, "topic">,
  ): Promise<ObjectsDto | null> {
    this.logger.log(`[ObjectsService] updateObject id=${id} unit=${unitId}`);

    this.validateObjectDto(dto, unitId);

    const topic = `${dto.type}/${unitId}/${id}`;
    const saved = await this.objectsRepository.update(id, unitId, {
      name: dto.name,
      type: dto.type,
      spec: dto.spec,
      description: dto.description,
      topic,
    });
    return saved ? toObjectsDto(saved) : null;
  }

  async deleteObject(id: string, unitId: string): Promise<boolean> {
    this.logger.log(`[ObjectsService] deleteObject id=${id} unit=${unitId}`);

    return this.objectsRepository.delete(id, unitId);
  }

  private validateObjectDto(dto: Omit<ObjectsDto, "topic">, unitId: string): void {
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
  }
}
