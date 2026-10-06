import { ObjectItem } from "../components";
import { formatRelativeTime } from "./format-relative-time";

export const transformObjectToCard = (obj: ObjectItem) => {
  const relative = formatRelativeTime(obj.updatedAt);
  const meta = relative
    ? `Последнее сообщение: ${relative}${obj.status === 'offline' ? ' · offline' : ''}`
    : undefined;

  return {
    title: obj.name,
    describe: obj.description || '',
    values: (obj.spec || [])
      .filter(s => s.value != null)
      .map(s => `${s.value}${s.spec.unit ? ` ${s.spec.unit}` : ''}`),
    meta,
  };
};
