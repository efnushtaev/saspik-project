export type MockObject = {
  id: string;
  name: string;
  type: 'sensor' | 'device';
  spec: {
    key: string;
    value: string | number | boolean | null;
    spec: { model: string; unit?: string; minorPart?: number };
  }[];
  description?: string;
  topic?: string;
};

// Mock data for objects
export const mockObjects: MockObject[] = [
  {
    id: 's6',
    name: 'DHT22',
    type: 'sensor' as const,
    spec: [
      { key: 'temperature', value: '24.40', spec: { model: 'dht22', unit: '°C', minorPart: 2 } },
      { key: 'humidity', value: '51.70', spec: { model: 'dht22', unit: '%', minorPart: 2 } },
    ],
    description: 'Датчик температуры и влажности',
  },
  {
    id: 'd1',
    name: 'LED',
    type: 'device' as const,
    spec: [
      { key: 'state', value: 'OFF', spec: { model: 'led', unit: '' } },
    ],
    description: 'Светодиодный индикатор',
    topic: 'led/control',
  },
];

// Mock data for units
export const mockUnits = [
  {
    id: 'unit-1',
    name: 'Производственный цех',
    description: 'Основной производственный цех завода',
    objects: mockObjects,
    rules: [],
  },
  {
    id: 'unit-2',
    name: 'Складская зона',
    description: 'Зона хранения готовой продукции',
    objects: [],
    rules: [],
  },
  {
    id: 'unit-3',
    name: 'Административное здание',
    description: 'Офисные помещения и административные службы',
    objects: [],
    rules: [],
  },
  {
    id: 'unit-4',
    name: 'Энергетический комплекс',
    description: 'Электроподстанция и котельная',
    objects: [],
    rules: [],
  },
];
