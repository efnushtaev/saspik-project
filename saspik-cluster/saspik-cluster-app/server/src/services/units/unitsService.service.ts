import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IUnitsService } from ".";
import { UnitDto } from "../../dto/units.dto";
import { TYPES, TEMPORARY_ANY } from "../../types";
import { IMqttService } from "../mqtt";
import {
  ObjectsRepository,
  RulesRepository,
  UnitsRepository,
  toObjectsDto,
  toRuleDto,
} from "../data-store";

@injectable()
export class UnitsService implements IUnitsService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
    @inject(TYPES.UnitsRepository) private unitsRepository: UnitsRepository,
    @inject(TYPES.ObjectsRepository) private objectsRepository: ObjectsRepository,
    @inject(TYPES.RulesRepository) private rulesRepository: RulesRepository,
  ) {
    this.logger.log("[UnitsService] initialized");
  }

  async getUnits(): Promise<UnitDto[]> {
    this.logger.log("[UnitsService] getUnits called");

    const [units, objects, rules] = await Promise.all([
      this.unitsRepository.findAll(),
      this.objectsRepository.findAll(),
      this.rulesRepository.findAll(),
    ]);
    const ruleDtos = rules.map(toRuleDto);

    return units.map((unit) => ({
      id: unit.id,
      name: unit.name,
      description: unit.description,
      objects: objects
        .filter((o) => o.unitId === unit.id)
        .map(toObjectsDto),
      rules: ruleDtos,
    }));
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
