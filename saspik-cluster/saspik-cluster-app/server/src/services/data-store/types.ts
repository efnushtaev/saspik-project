import { ObjectsDto } from "../../dto/objects.dto";
import { RuleDto } from "../../dto/rules.dto";

export interface UnitEntity {
  id: string;
  name: string;
  description?: string;
}

export interface ObjectEntity extends ObjectsDto {
  unitId: string;
}

export interface RuleEntity {
  id: string;
  name?: string;
  unitId?: string;
  trigger: { topic: string | string[]; qos?: 0 | 1 | 2 };
  when?: unknown;
  then: unknown[];
  enabled?: boolean;
  [key: string]: unknown;
}
