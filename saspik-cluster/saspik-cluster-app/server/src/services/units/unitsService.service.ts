import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IUnitsService } from ".";
import { UnitDto } from "../../dto/units.dto";
import { ObjectsType } from "../../dto/objects.dto";
import { TYPES, TEMPORARY_ANY } from "../../types";
import { IMqttService } from "../mqtt";

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
        id: "u1",
        name: "Main Greenhouse",
        description: "Primary greenhouse climate control",
        objects: [
          {
            id: "s6",
            name: "DHT22",
            type: ObjectsType.SENSOR,
            topic: "sensors/dht22",
            spec: [
              { key: "temperature", model: "dht22", unit: "℃" },
              { key: "humidity", model: "dht22", unit: "%" },
            ],
          },
          {
            id: "d1",
            name: "LED",
            type: ObjectsType.DEVICE,
            topic: "led/control",
            spec: [
              { key: "state", model: "led", unit: "" },
            ],
            description: "Светодиодный индикатор",
          },
        ],
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
