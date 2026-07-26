import { ObjectsDto } from "./objects.dto";
import { RuleDto } from "./rules.dto";

export class UnitDto {
  id: string;
  name: string;
  description?: string;
  objects: ObjectsDto[];
  rules: RuleDto[];
}
