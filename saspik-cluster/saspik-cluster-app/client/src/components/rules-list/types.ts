export interface RuleItem {
  id: string;
  name?: string;
  unitId?: string;
  trigger: { topic: string | string[]; qos?: 0 | 1 | 2 };
  when?: unknown;
  then: { action: string; params?: Record<string, unknown> }[];
  enabled: boolean;
}

export interface RulesListProps {
  unitId?: string;
}