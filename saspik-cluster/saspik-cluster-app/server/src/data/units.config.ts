import { ObjectsDto } from "../dto/objects.dto";
import { unitId1Objects } from "./unitId1.config";
import { unitId2Objects } from "./unitId2.config";

export interface UnitConfig {
  id: string;
  name: string;
  description: string;
  objects: ObjectsDto[];
}

export const unitsConfig: UnitConfig[] = [
  {
    id: "unitId1",
    name: "ESP32 Controller",
    description: "ESP32 controller with sensors and relays",
    objects: unitId1Objects,
  },
  {
    id: "unitId2",
    name: "ESP32 Local MQTT",
    description: "ESP32 local MQTT (DHT22 + LED)",
    objects: unitId2Objects,
  },
];

export function getAllObjects(): ObjectsDto[] {
  return unitsConfig.flatMap((unit) => unit.objects);
}

export function getObjectsByUnit(unitId?: string): ObjectsDto[] {
  if (!unitId) return getAllObjects();
  const unit = unitsConfig.find((u) => u.id === unitId);
  return unit ? unit.objects : [];
}
