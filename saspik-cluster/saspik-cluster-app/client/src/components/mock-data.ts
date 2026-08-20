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
export type MockRule = {
  id: string;
  name?: string;
  unitId?: string;
  trigger: { topic: string | string[]; qos?: 0 | 1 | 2 };
  when?: unknown;
  then: { action: string; params?: Record<string, unknown> }[];
  enabled: boolean;
};

export type MockUnit = {
  id: string;
  name: string;
  description?: string;
  objects: MockObject[];
  rules: MockRule[];
};

export const mockRules: MockRule[] = [
  {
    id: 'rule-1',
    name: 'Аварийный высокий порог температуры',
    unitId: 'unit-1',
    trigger: { topic: 'sensor/unit-1/dht22', qos: 0 },
    when: { jsonpath: '$.temperature > 30' },
    then: [
      { action: 'publish', params: { topic: 'units/unit-1/commands/a_relay3', payload: '{"state":"1"}' } },
    ],
    enabled: true,
  },
  {
    id: 'rule-2',
    name: 'Аварийный низкий порог температуры',
    unitId: 'unit-1',
    trigger: { topic: 'sensor/unit-1/dht22', qos: 0 },
    when: { jsonpath: '$.temperature < 10' },
    then: [
      { action: 'publish', params: { topic: 'units/unit-1/commands/a_relay4', payload: '{"state":"1"}' } },
    ],
    enabled: false,
  },
  {
    id: 'rule-3',
    name: 'Выключить вентилятор при превышении влажности',
    unitId: 'unit-2',
    trigger: { topic: ['sensor/unit-2/dht22', 'units/unit-2/commands/a_relay3'] },
    when: { jsonpath: '$.humidity > 80' },
    then: [
      { action: 'publish', params: { topic: 'units/unit-2/commands/a_relay3', payload: '{"state":"0"}' } },
    ],
    enabled: true,
  },
];

export const mockUnits: MockUnit[] = [
  {
    id: 'unit-1',
    name: 'Производственный цех',
    description: 'Основной производственный цех завода',
    objects: mockObjects,
    rules: mockRules.filter(r => r.unitId === 'unit-1'),
  },
  {
    id: 'unit-2',
    name: 'Складская зона',
    description: 'Зона хранения готовой продукции',
    objects: [],
    rules: mockRules.filter(r => r.unitId === 'unit-2'),
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
