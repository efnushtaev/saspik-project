import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IUnitsService } from ".";
import { UnitDto } from "../../dto/units.dto";
import { TYPES, TEMPORARY_ANY } from "../../types";
import { IMqttService } from "../mqtt";
import { unitId1Objects } from "../../data/unitId1.config";

@injectable()
export class UnitsService implements IUnitsService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
  ) {
    this.logger.log("[UnitsService] initialized");
  }

  async getUnits(): Promise<UnitDto[]> {
    this.logger.log("[UnitsService] getUnits called");

    const mockRules = [
      { id: "r1", name: "High temp alert", condition: "temperature > 30", action: "notify", enabled: true },
      { id: "r2", name: "Low humidity", condition: "humidity < 30", action: "humidifier_on", enabled: true },
    ];

    return [
      {
        id: "unitId1",
        name: "ESP32 Controller",
        description: "ESP32 controller with sensors and relays",
        objects: unitId1Objects,
        rules: mockRules,
      },
    ];
  }

  async callCommand(
    unitId: string,
    command: string,
    payload?: TEMPORARY_ANY,
  ): Promise<void> {
    this.logger.log(
      `[UnitsService] callCommand for unit ${unitId}, command ${command}`,
      payload,
    );
    const topic = `units/${unitId}/commands/${command}`;
    const message = payload ? JSON.stringify(payload) : "";
    await this.mqttService.publish(topic, message);
  }
}
