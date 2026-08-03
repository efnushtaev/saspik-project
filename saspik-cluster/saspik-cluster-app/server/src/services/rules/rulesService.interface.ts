import { RuleDto } from "../../dto/rules.dto";

export interface IRulesService {
  getRules(): Promise<RuleDto[]>;
  getRule(id: string): Promise<RuleDto | null>;
  upsertRule(rule: RuleDto): Promise<RuleDto>;
  setEnabled(id: string, enabled: boolean): Promise<RuleDto | null>;
  deleteRule(id: string): Promise<boolean>;
}
