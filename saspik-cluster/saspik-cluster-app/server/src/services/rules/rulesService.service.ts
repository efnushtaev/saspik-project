import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IRulesService } from ".";
import { RuleDto } from "../../dto/rules.dto";
import { TYPES } from "../../types";
import { RulesRepository, toRuleDto, RuleEntity } from "../data-store";

@injectable()
export class RulesService implements IRulesService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.RulesRepository) private rulesRepository: RulesRepository,
  ) {
    this.logger.log("[RulesService] initialized");
  }

  async getRules(unitId?: string): Promise<RuleDto[]> {
    const entities = await this.rulesRepository.findByUnitId(unitId);
    return entities.map(toRuleDto);
  }

  async getRule(id: string): Promise<RuleDto | null> {
    const entity = await this.rulesRepository.findById(id);
    return entity ? toRuleDto(entity) : null;
  }

  async upsertRule(rule: RuleDto): Promise<RuleDto> {
    if (!rule.id || !rule.trigger || !Array.isArray(rule.then)) {
      throw new Error("Rule must have id, trigger and then");
    }
    const entity: RuleEntity = {
      ...rule,
      enabled: rule.enabled ?? true,
    };
    const saved = await this.rulesRepository.upsert(entity);
    return toRuleDto(saved);
  }

  async setEnabled(id: string, enabled: boolean): Promise<RuleDto | null> {
    const updated = await this.rulesRepository.update(id, { enabled });
    return updated ? toRuleDto(updated) : null;
  }

  async deleteRule(id: string): Promise<boolean> {
    return this.rulesRepository.delete(id);
  }
}
