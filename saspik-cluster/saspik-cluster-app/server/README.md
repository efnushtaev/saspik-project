[↑ ATSAP Cluster App](../README.md)

# ATSAP Cluster Server

## Общее описание

Серверное приложение, реализующее REST API для работы с юнитами и объектами. Читает последние значения объектов из InfluxDB (данные поступают через Telegraf из MQTT-брокера). Построено на Express.js с Dependency Injection (InversifyJS).

##### Основные компоненты:
- **Контроллеры** — обработка HTTP запросов
- **Сервисы** — бизнес-логика
- **State Store** — хранилище последних значений объектов в InfluxDB (`InfluxDbStateStoreService`, интерфейс `IStateStoreService`)
- **DTO** — объекты передачи данных

##### Технические детали:
- TypeScript, Express.js, порт 3001
- Dependency Injection (InversifyJS)
- class-validator, tslog

---

## API

### Внешние API

1. **Rightech API** — получение данных об объектах и моделях
   - Базовый URL: `https://dev.rightech.io/api/v1`
   - Авторизация: Bearer токен (`RIGHTECH_API_TOKEN`)

### Внутренний API

#### 1. Health check
- `GET /health` → `"OK"`

#### 2. Timestamp
- `GET /api/getTimestamp` → `{ "timestamp": "2026-06-29T12:00:00.000Z" }`

#### 3. Список юнитов
- `GET /api/v1/units/list`
- Ответ (мок):
  ```json
  {
    "units": [
      {
        "id": "unitId1",
        "name": "ESP32 Controller",
        "description": "ESP32 controller with sensors and relays",
        "objects": [
          { "id": "dht22", "name": "DHT22", "type": "sensor", "topic": "sensors/dht22", "spec": [{ "key": "temperature", "model": "dht22", "unit": "℃" }, { "key": "humidity", "model": "dht22", "unit": "%" }] },
          { "id": "float-1", "name": "Float Sensor", "type": "sensor", "topic": "sensors/float-1", "spec": [{ "key": "floatSensor", "model": "float", "unit": "" }] },
          { "id": "a_relay1", "name": "Light", "type": "device", "topic": "units/unitId1/commands/a_relay1", "spec": [{ "key": "state", "model": "relay", "unit": "" }], "description": "Свет (реле 1)" },
          { "id": "a_relay2", "name": "Humidifier", "type": "device", "topic": "units/unitId1/commands/a_relay2", "spec": [{ "key": "state", "model": "relay", "unit": "" }], "description": "Увлажнитель (реле 2)" },
          { "id": "a_relay3", "name": "Fan", "type": "device", "topic": "units/unitId1/commands/a_relay3", "spec": [{ "key": "state", "model": "relay", "unit": "" }], "description": "Вентилятор (реле 3)" },
          { "id": "a_relay4", "name": "Water Pump", "type": "device", "topic": "units/unitId1/commands/a_relay4", "spec": [{ "key": "state", "model": "relay", "unit": "" }], "description": "Насос (реле 4)" }
        ],
        "rules": [
          { "id": "r1", "name": "High temp alert", "condition": "temperature > 30", "action": "notify", "enabled": true },
          { "id": "r2", "name": "Low humidity", "condition": "humidity < 30", "action": "humidifier_on", "enabled": true }
        ]
      },
      {
        "id": "unitId2",
        "name": "ESP32 Local MQTT",
        "description": "ESP32 local MQTT (DHT22 + LED)",
        "objects": [
          { "id": "dht22", "name": "DHT22", "type": "sensor", "topic": "sensors/unitId2/dht22", "spec": [{ "key": "temperature", "model": "dht22", "unit": "℃" }, { "key": "humidity", "model": "dht22", "unit": "%" }] },
          { "id": "led", "name": "LED", "type": "device", "topic": "led/control", "spec": [{ "key": "state", "model": "led", "unit": "" }], "description": "Светодиодный индикатор" }
        ],
        "rules": []
      }
    ]
  }
  ```

#### 4. Список объектов по типу
- `POST /api/v1/objects/list/:type`
- `:type` — `sensor` или `device`
- Тело запроса (опционально): `{ "unitId": "unitId2" }` — фильтр по юниту; если не указан, возвращаются объекты всех юнитов
- `value` загружается из InfluxDB (последнее показание) по топику объекта
- Форматирование: значение округляется по `spec.minorPart` (если задан), возвращается строкой
- Ответ (sensor):
  ```json
  {
    "objects": [
      {
        "id": "dht22",
        "name": "DHT22",
        "type": "sensor",
        "topic": "sensors/dht22",
        "spec": [
          { "key": "temperature", "value": 23.6, "spec": { "key": "temperature", "model": "dht22", "unit": "℃" } },
          { "key": "humidity", "value": 46.9, "spec": { "key": "humidity", "model": "dht22", "unit": "%" } }
        ]
      }
    ]
  }
  ```
- Ответ (device):
  ```json
  {
    "objects": [
      {
        "id": "a_relay1",
        "name": "Light",
        "type": "device",
        "topic": "units/unitId1/commands/a_relay1",
        "spec": [
          { "key": "state", "value": "1", "spec": { "key": "state", "model": "relay", "unit": "" } }
        ],
        "description": "Свет (реле 1)"
      }
    ]
  }
  ```

#### 5. Объекты по IDs
- `POST /api/v1/objects/getByIds`
- Тело: `{ "id": ["dht22", "a_relay1"], "type": "sensor" }`
- `type` — опционально, `sensor` или `device` (фильтр по типу)
- `unitId` — опционально, фильтр по юниту; если не указан, поиск по всем юнитам
- Ответ:
  ```json
  {
    "objects": [
      {
        "id": "dht22",
        "name": "DHT22",
        "type": "sensor",
        "topic": "sensors/dht22",
        "spec": [
          { "key": "temperature", "value": 23.6, "spec": { "key": "temperature", "model": "dht22", "unit": "℃" } },
          { "key": "humidity", "value": 46.9, "spec": { "key": "humidity", "model": "dht22", "unit": "%" } }
        ]
      }
    ]
  }
  ```

#### 6. Команда устройству
- `POST /api/v1/objects/command/:deviceId`
- Тело: `{ "value": "1" }`; `unitId` — опционально, ищет устройство в рамках юнита (если не указан — по всем юнитам)
- Ответ: `{ "success": true }`

#### 7. Последние показания сенсоров
- `POST /api/v1/objects/getLastSensorsData`
- Тело: `{ "id": ["s6", "s7"] }`
- Ответ: `{ "s6": "24.40", "s7": "51.70" }` (заглушка, возвращает `{}`)

#### 8. MQTT Publish
- `POST /api/v1/mqtt/publish`
- Тело: `{ "topic": "test", "message": "hello", "qos": 0, "retain": false }`

#### 9. MQTT Subscribe
- `POST /api/v1/mqtt/subscribe`
- Тело: `{ "topic": "test" }`

#### 10. MQTT Unsubscribe
- `POST /api/v1/mqtt/unsubscribe`
- Тело: `{ "topic": "test" }`

---

## Переменные окружения

```
RIGHTECH_API_TOKEN=your_token
PORT=3001

# InfluxDB
INFLUXDB_URL=http://localhost:8086
INFLUXDB_TOKEN=my-super-secret-token
INFLUXDB_ORG=saspik
INFLUXDB_BUCKET=mqtt
```

## Структура каталога

```
src/
├── app.ts                        # Точка входа, настройка Express
├── index.ts                      # IoC контейнер (Inversify)
├── const.ts                      # Константы маршрутов
├── types.ts                      # Типы DI
├── common/                       # BaseController, route interface
├── config/                       # ConfigService
├── controllers/                  # Units, Objects, MQTT контроллеры
├── data/                         # Конфиги юнитов (unitId1.config.ts, unitId2.config.ts, units.config.ts — реестр юнитов для ObjectsService и UnitsService)
├── dto/                          # ObjectsDto, UnitDto, RuleDto
├── errors/                       # ExceptionFilter
├── logger/                       # LoggerService
├── services/
│   ├── objects/                  # ObjectsService (бизнес-логика объектов)
│   ├── units/                    # UnitsService
│   ├── state-store/              # IStateStoreService + InfluxDbStateStoreService + InMemoryStateStoreService
│   ├── mqtt/                     # LocalMqttService, MqttService
│   ├── climate-control/          # ClimateControlService
│   └── mysql/                    # MySQLService
└── dto/                          # RightechObjectDto, RightechModelDto

../shared/                         # Общие TypeScript-типы (подключаются через @shared/*)
```
