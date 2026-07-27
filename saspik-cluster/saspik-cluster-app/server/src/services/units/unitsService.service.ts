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
        id: "unitId1",
        name: "ESP32 Controller",
        description: "ESP32 controller with sensors and relays",
        objects: [
          {
            id: "dht22",
            name: "DHT22",
            type: ObjectsType.SENSOR,
            topic: "sensors/dht22",
            spec: [
              { key: "temperature", model: "dht22", unit: "℃" },
              { key: "humidity", model: "dht22", unit: "%" },
            ],
          },
          {
            id: "float-1",
            name: "Float Sensor",
            type: ObjectsType.SENSOR,
            topic: "sensors/float-1",
            spec: [
              { key: "floatSensor", model: "float", unit: "" },
            ],
          },
          {
            id: "a_relay1",
            name: "Light",
            type: ObjectsType.DEVICE,
            topic: "units/unitId1/commands/a_relay1",
            spec: [
              { key: "state", model: "relay", unit: "" },
            ],
            description: "Свет (реле 1)",
          },
          {
            id: "a_relay2",
            name: "Humidifier",
            type: ObjectsType.DEVICE,
            topic: "units/unitId1/commands/a_relay2",
            spec: [
              { key: "state", model: "relay", unit: "" },
            ],
            description: "Увлажнитель (реле 2)",
          },
          {
            id: "a_relay3",
            name: "Fan",
            type: ObjectsType.DEVICE,
            topic: "units/unitId1/commands/a_relay3",
            spec: [
              { key: "state", model: "relay", unit: "" },
            ],
            description: "Вентилятор (реле 3)",
          },
          {
            id: "a_relay4",
            name: "Water Pump",
            type: ObjectsType.DEVICE,
            topic: "units/unitId1/commands/a_relay4",
            spec: [
              { key: "state", model: "relay", unit: "" },
            ],
            description: "Насос (реле 4)",
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
