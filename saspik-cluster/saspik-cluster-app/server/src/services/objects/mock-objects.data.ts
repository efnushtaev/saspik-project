import {
  ObjectsDto,
  ObjectsType,
} from "../../dto/objects.dto";

export const mockObjects: ObjectsDto[] = [
  {
    id: "s6",
    name: "DHT22",
    type: ObjectsType.SENSOR,
    topic: "sensors/dht22",
    spec: [
      { key: "temperature", model: "dht22", unit: "\u2103", minorPart: 1 },
      { key: "humidity", model: "dht22", unit: "%", minorPart: 1 },
    ],
  },
  {
    id: "d1",
    name: "LED",
    type: ObjectsType.DEVICE,
    topic: "led/control",
    spec: [
      { key: "state", model: "led", unit: "" },
    ],
    description: "Светодиодный индикатор",
  },
];
