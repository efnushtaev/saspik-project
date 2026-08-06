import { ObjectsDto, ObjectsType } from "../dto/objects.dto";

export const unitId1Objects: ObjectsDto[] = [
  {
    id: "dht22",
    name: "DHT22",
    type: ObjectsType.SENSOR,
    topic: "sensor/unitId1/dht22",
    spec: [
      { key: "temperature", model: "dht22", unit: "℃" },
      { key: "humidity", model: "dht22", unit: "%" },
    ],
  },
  {
    id: "float-1",
    name: "Float Sensor",
    type: ObjectsType.SENSOR,
    topic: "sensor/unitId1/float-1",
    spec: [
      { key: "floatSensor", model: "float", unit: "" },
    ],
  },
  {
    id: "a_relay1",
    name: "Light",
    type: ObjectsType.DEVICE,
    topic: "units/unitId1/commands/a_relay1",
    spec: [
      { key: "state", model: "relay", unit: "" },
    ],
    description: "Свет (реле 1)",
  },
  {
    id: "a_relay2",
    name: "Humidifier",
    type: ObjectsType.DEVICE,
    topic: "units/unitId1/commands/a_relay2",
    spec: [
      { key: "state", model: "relay", unit: "" },
    ],
    description: "Увлажнитель (реле 2)",
  },
  {
    id: "a_relay3",
    name: "Fan",
    type: ObjectsType.DEVICE,
    topic: "units/unitId1/commands/a_relay3",
    spec: [
      { key: "state", model: "relay", unit: "" },
    ],
    description: "Вентилятор (реле 3)",
  },
  {
    id: "a_relay4",
    name: "Water Pump",
    type: ObjectsType.DEVICE,
    topic: "units/unitId1/commands/a_relay4",
    spec: [
      { key: "state", model: "relay", unit: "" },
    ],
    description: "Насос (реле 4)",
  },
];
