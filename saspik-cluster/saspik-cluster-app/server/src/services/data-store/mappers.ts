import { ObjectsDto } from "../../dto/objects.dto";
import { RuleDto, RuleActionDto } from "../../dto/rules.dto";
import { ObjectEntity, RuleEntity } from "./types";

export function toObjectsDto(o: ObjectEntity): ObjectsDto {
  return {
    id: o.id,
    name: o.name,
    type: o.type,
    spec: o.spec,
    description: o.description,
    topic: o.topic,
  };
}

export function toRuleDto(r: RuleEntity): RuleDto {
  return {
    id: r.id,
    name: r.name,
    unitId: r.unitId,
    trigger: r.trigger,
    when: r.when,
    then: (r.then as RuleActionDto[]) ?? [],
    enabled: r.enabled ?? true,
  };
}
