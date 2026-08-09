import { ObjectsDto } from "./objects.dto";
import { RuleDto } from "./rules.dto";

export class UnitDto {
  id: string;
  name: string;
  description?: string;
  objects: ObjectsDto[];
  rules: RuleDto[];
}

export class CreateUnitDto {
  id: string;
  name: string;
  description?: string;
}

export class UpdateUnitDto {
  name: string;
  description?: string;
}
