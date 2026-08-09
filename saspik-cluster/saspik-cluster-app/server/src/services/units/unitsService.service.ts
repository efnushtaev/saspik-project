import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IUnitsService } from ".";
import { UnitDto, CreateUnitDto, UpdateUnitDto } from "../../dto/units.dto";
import { RuleDto } from "../../dto/rules.dto";
import { TYPES, TEMPORARY_ANY } from "../../types";
import { IMqttService } from "../mqtt";
import {
  ObjectsRepository,
  RulesRepository,
  UnitsRepository,
  UnitEntity,
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

    return units.map((unit) => this.toUnitDto(unit, objects, ruleDtos));
  }

  async getUnitById(id: string): Promise<UnitDto | null> {
    this.logger.log(`[UnitsService] getUnitById called id=${id}`);

    const unit = await this.unitsRepository.findById(id);
    if (!unit) {
      return null;
    }

    const [objects, rules] = await Promise.all([
      this.objectsRepository.findAll(),
      this.rulesRepository.findAll(),
    ]);

    return this.toUnitDto(unit, objects, rules.map(toRuleDto));
  }

  async createUnit(dto: CreateUnitDto): Promise<UnitDto> {
    this.logger.log(`[UnitsService] createUnit id=${dto.id}`);

    this.validateUnitDto(dto);

    const entity: UnitEntity = {
      id: dto.id.trim(),
      name: dto.name.trim(),
      description: dto.description?.trim() || undefined,
    };
    await this.unitsRepository.create(entity);
    return this.toUnitDto(entity, [], []);
  }

  async updateUnit(id: string, dto: UpdateUnitDto): Promise<UnitDto | null> {
    this.logger.log(`[UnitsService] updateUnit id=${id}`);

    if (!dto.name || !dto.name.trim()) {
      throw new Error("Unit name is required");
    }

    const saved = await this.unitsRepository.update(id, {
      name: dto.name.trim(),
      description: dto.description?.trim() || undefined,
    });
    if (!saved) {
      return null;
    }

    const [objects, rules] = await Promise.all([
      this.objectsRepository.findAll(),
      this.rulesRepository.findAll(),
    ]);

    return this.toUnitDto(saved, objects, rules.map(toRuleDto));
  }

  async deleteUnit(id: string): Promise<boolean> {
    this.logger.log(`[UnitsService] deleteUnit id=${id}`);

    const deletedObjects = await this.objectsRepository.deleteByUnitId(id);
    if (deletedObjects > 0) {
      this.logger.log(
        `[UnitsService] cascade deleted ${deletedObjects} objects for unit ${id}`,
      );
    }

    return this.unitsRepository.delete(id);
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

  private toUnitDto(
    unit: UnitEntity,
    objects: Awaited<ReturnType<ObjectsRepository["findAll"]>>,
    rules: RuleDto[],
  ): UnitDto {
    return {
      id: unit.id,
      name: unit.name,
      description: unit.description,
      objects: objects
        .filter((o) => o.unitId === unit.id)
        .map(toObjectsDto),
      rules,
    };
  }

  private validateUnitDto(dto: CreateUnitDto): void {
    if (!dto.id || !dto.id.trim()) {
      throw new Error("Unit id is required");
    }
    if (!dto.name || !dto.name.trim()) {
      throw new Error("Unit name is required");
    }
  }
}
