export interface RuleTriggerDto {
  topic: string | string[];
  qos?: 0 | 1 | 2;
}

export interface RuleActionDto {
  action: string;
  params?: Record<string, unknown>;
}

export class RuleDto {
  id: string;
  name?: string;
  unitId?: string;
  trigger: RuleTriggerDto;
  when?: unknown;
  then: RuleActionDto[];
  enabled: boolean;
}
