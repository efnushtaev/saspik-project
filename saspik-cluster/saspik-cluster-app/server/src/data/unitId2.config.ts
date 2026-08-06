import { ObjectsDto, ObjectsType } from "../dto/objects.dto";

export const unitId2Objects: ObjectsDto[] = [
  {
    id: "dht22",
    name: "DHT22",
    type: ObjectsType.SENSOR,
    topic: "sensor/unitId2/dht22",
    spec: [
      { key: "temperature", model: "dht22", unit: "℃" },
      { key: "humidity", model: "dht22", unit: "%" },
    ],
  },
  {
    id: "led",
    name: "LED",
    type: ObjectsType.DEVICE,
    topic: "led/control",
    spec: [
      { key: "state", model: "led", unit: "" },
    ],
    description: "Светодиодный индикатор",
  },
];
