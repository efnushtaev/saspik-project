export interface UnitFormValues {
  unitId: string;
  name: string;
  description: string;
}

export const emptyUnitFormValues = (): UnitFormValues => ({
  unitId: '',
  name: '',
  description: '',
});
