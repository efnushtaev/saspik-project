import { ObjectItem } from "../components";

export const transformObjectToCard = (obj: ObjectItem) => ({
  title: obj.name,
  describe: obj.description || '',
  values: (obj.spec || [])
    .filter(s => s.value != null)
    .map(s => `${s.value}${s.spec.unit ? ` ${s.spec.unit}` : ''}`),
});
