[↑ ATSAP Cluster App](../README.md)

# ATSAP Cluster Server

## Общее описание

Серверное приложение, реализующее REST API для работы с юнитами, объектами и правилами. Юниты, объекты и правила хранятся в MongoDB (коллекции `units`, `objects`, `rules`); последние значения объектов читаются из InfluxDB (данные поступают через Telegraf из MQTT-брокера). Построено на Express.js с Dependency Injection (InversifyJS).

##### Основные компоненты:
- **Контроллеры** — обработка HTTP запросов
- **Сервисы** — бизнес-логика
- **Data Store** — слой доступа к MongoDB (`MongoService`, репозитории `UnitsRepository`/`ObjectsRepository`/`RulesRepository`, `SeedService`)
- **State Store** — хранилище последних значений объектов в InfluxDB (`InfluxDbStateStoreService`, интерфейс `IStateStoreService`)
- **DTO** — объекты передачи данных

##### Технические детали:
- TypeScript, Express.js, порт 3001
- Dependency Injection (InversifyJS)
- MongoDB (официальный драйвер `mongodb`), class-validator, tslog

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
- Данные юнитов и объектов загружаются из MongoDB (коллекции `units`, `objects`), правила — из коллекции `rules` в формате rule-engine. При первой инициализации БД заполняется сидами из `server/src/data/*.config.ts` и `server/data/rules.json`.
- Ответ:
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
          {
            "id": "temp_emergency_high",
            "trigger": { "topic": "sensors/dht22", "qos": 0 },
            "when": { "jsonpath": "$.temperature > 29" },
            "then": [
              { "action": "publish", "params": { "topic": "units/unitId1/commands/a_relay3", "payload": "{\"state\":\"1\"}", "qos": 1 } },
              { "action": "publish", "params": { "topic": "units/unitId1/commands/a_relay2", "payload": "{\"state\":\"1\"}", "qos": 1 } }
            ],
            "enabled": true
          }
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
  Правила возвращаются в формате rule-engine (`trigger`/`when`/`then`/`enabled`). В примере показано одно правило; фактически возвращаются все правила из коллекции `rules`.

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

#### 7.1 Создание объекта
- `POST /api/v1/objects`
- Тело: `{ "id", "name", "type", "spec", "description?", "unitId" }` (без `topic` — топик формируется сервером по паттерну `` `${type}/${unitId}/${id}` ``)
- `spec` — массив записей: `{ "key", "model", "unit"? , "minorPart"? }`
- Ответ: `{ "object": { ... } }`. 400 при невалидных данных или дубликате (`id` + `unitId`).

#### 8. Правила
Управление правилами автоматизации (формат rule-engine: `trigger`/`when`/`then`/`enabled`). Хранятся в коллекции `rules`.

- `GET /api/v1/rules` — список всех правил. Ответ: `{ "rules": [...] }`
- `POST /api/v1/rules` — создание/обновление правила по `id` (upsert). Тело — правило в формате rule-engine. Ответ: `{ "rule": { ... } }`
  ```json
  {
    "id": "temp_emergency_high",
    "trigger": { "topic": "sensors/dht22", "qos": 0 },
    "when": { "jsonpath": "$.temperature > 29" },
    "then": [ { "action": "publish", "params": { "topic": "units/unitId1/commands/a_relay3", "payload": "{\"state\":\"1\"}", "qos": 1 } } ],
    "enabled": true
  }
  ```
- `PATCH /api/v1/rules/:id` — включить/отключить правило. Тело: `{ "enabled": false }`. Ответ: `{ "rule": { ... } }`. Отключённые правила (`enabled: false`) не выполняются движком.
- `DELETE /api/v1/rules/:id` — удалить правило. Ответ: `{ "success": true }` (404, если правило не найдено).

#### 9. MQTT Publish
- `POST /api/v1/mqtt/publish`
- Тело: `{ "topic": "test", "message": "hello", "qos": 0, "retain": false }`

#### 10. MQTT Subscribe
- `POST /api/v1/mqtt/subscribe`
- Тело: `{ "topic": "test" }`

#### 11. MQTT Unsubscribe
- `POST /api/v1/mqtt/unsubscribe`
- Тело: `{ "topic": "test" }`

---

## Переменные окружения

```
RIGHTECH_API_TOKEN=your_token
PORT=3001

# MongoDB
MONGODB_URL=mongodb://mongo:27017/saspik

# InfluxDB
INFLUXDB_URL=http://localhost:8086
INFLUXDB_TOKEN=my-super-secret-token
INFLUXDB_ORG=saspik
INFLUXDB_BUCKET=mqtt

# Seed правил (путь к файлу-источнику для первичного наполнения коллекции rules)
INIT_RULES_SEED_PATH=./data/rules.json
```

При первом запуске (пустая БД) `SeedService` заполняет коллекции `units`, `objects`, `rules` из `src/data/*.config.ts` и файла `INIT_RULES_SEED_PATH` (env-подстановка `${VAR}` и `${expr:...}` выполняется до записи в БД). Сид идемпотентен: непустые коллекции не перезаполняются.

## Структура каталога

```
src/
├── app.ts                        # Точка входа, настройка Express
├── index.ts                      # IoC контейнер (Inversify)
├── const.ts                      # Константы маршрутов
├── types.ts                      # Типы DI
├── common/                       # BaseController, route interface
├── config/                       # ConfigService
├── controllers/                  # Units, Objects, Rules, MQTT контроллеры
├── data/                         # Конфиги юнитов (unitId1.config.ts, unitId2.config.ts, units.config.ts) и rules.json — источники сидов
├── dto/                          # ObjectsDto, UnitDto, RuleDto
├── errors/                       # ExceptionFilter
├── logger/                       # LoggerService
├── services/
│   ├── objects/                  # ObjectsService (бизнес-логика объектов)
│   ├── units/                    # UnitsService
│   ├── rules/                    # RulesService (управление правилами через API)
│   ├── data-store/               # MongoService, Units/Objects/RulesRepository, SeedService
│   ├── state-store/              # IStateStoreService + InfluxDbStateStoreService + InMemoryStateStoreService
│   ├── mqtt/                     # LocalMqttService, MqttService
│   └── climate-control/          # ClimateControlService
└── dto/                          # RightechObjectDto, RightechModelDto

../shared/                         # Общие TypeScript-типы (подключаются через @shared/*)
```
