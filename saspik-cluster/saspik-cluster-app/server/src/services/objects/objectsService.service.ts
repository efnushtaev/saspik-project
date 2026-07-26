import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IObjectsService } from ".";
import { TYPES } from "../../types";
import { TEMPORARY_ANY } from "../../types";
import { mockObjects } from "./mock-objects.data";
import { IStateStoreService } from "../state-store/stateStore.interface";
import { IMqttService } from "../mqtt";
import { ObjectsType } from "../../dto/objects.dto";

@injectable()
export class ObjectsService implements IObjectsService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.StateStoreService) private stateStore: IStateStoreService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
  ) {
    this.logger.log("[ObjectsService] initialized");
  }

  async getObjects(typeFilter?: string): Promise<TEMPORARY_ANY[]> {
    this.logger.log(
      `[ObjectsService] getObjects${typeFilter ? ` filter=${typeFilter}` : ""}`,
    );

    if (!typeFilter) return mockObjects;

    return mockObjects.filter((obj) => obj.type === typeFilter);
  }

  async getByIds(ids: string[], typeFilter?: string): Promise<TEMPORARY_ANY[]> {
    this.logger.log(`[ObjectsService] getByIds`);

    let result = mockObjects;

    if (typeFilter) {
      result = result.filter((obj) => obj.type === typeFilter);
    }

    return result.filter((obj) => ids.includes(obj.id));
  }

  async callCommand(deviceId: string, value: string): Promise<void> {
    this.logger.log(
      `[ObjectsService] callCommand for device ${deviceId}, value ${value}`,
    );

    const device = mockObjects.find(
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

  async getObjectState(id: string, field?: string): Promise<number | string | boolean | null> {
    this.logger.log(`[ObjectsService] getObjectState id=${id} field=${field}`);

    const stored = await this.stateStore.get(id, field);
    return stored?.value ?? null;
  }
}
