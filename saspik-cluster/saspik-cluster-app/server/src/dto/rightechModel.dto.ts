type LevelsValue = {
  color: string;
  name: string;
  value: {
    a: string;
    b: number;
  };
};

type ModelChildren = {
  id: string;
  name: string;
  active: boolean;
  type: string;
  source?: string;
  dataType?: string;
  unit?: string;
  reference?: string;
  display?: object;
  factor?: number;
  children?: ModelChildren[];
  linear?: boolean;
  description?: string;
  levels?: {
    type: string;
    value: LevelsValue[];
    svg: string;
  };
  _inactive?: boolean;
  _parentId?: string;
};

export class RightechModelDto {
  _id: string;
  name: string;
  base: string;
  description: string;
  mixins: [];
  data: {
    id: string;
    name: string;
    active: boolean;
    type: string;
    children: ModelChildren[];
    _isRoot: boolean;
    _inactive: boolean;
    description: string;
  };
  group: string;
}
