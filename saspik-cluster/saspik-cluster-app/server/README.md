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
        "id": "u1",
        "name": "Main Greenhouse",
        "description": "Primary greenhouse climate control",
        "objects": [
          { "id": "s6", "name": "DHT22 Temperature", "type": "sensor", "topic": "sensors/dht22", "spec": { "model": "dht22", "unit": "℃" } },
          { "id": "s7", "name": "DHT22 Humidity", "type": "sensor", "topic": "sensors/dht22", "spec": { "model": "dht22", "unit": "%" } }
        ],
        "rules": [
          { "id": "r1", "name": "High temp alert", "condition": "temperature > 30", "action": "notify", "enabled": true },
          { "id": "r2", "name": "Low humidity", "condition": "humidity < 30", "action": "humidifier_on", "enabled": true }
        ]
      }
    ]
  }
  ```

#### 4. Список объектов по типу
- `POST /api/v1/objects/list/:type`
- `:type` — `sensor` или `device`
- Тело запроса: не требуется
- `value` загружается из InfluxDB (последнее показание)
- Форматирование: значение округляется по `spec.minorPart` (если задан), возвращается строкой
- Ответ:
  ```json
  {
    "objects": [
      {
        "id": "s6",
        "name": "DHT22 Temperature",
        "type": "sensor",
        "topic": "sensors/dht22",
        "value": "24.40",
        "spec": { "model": "dht22", "unit": "℃", "minorPart": 2 }
      },
      {
        "id": "s7",
        "name": "DHT22 Humidity",
        "type": "sensor",
        "topic": "sensors/dht22",
        "value": "51.70",
        "spec": { "model": "dht22", "unit": "%", "minorPart": 2 }
      }
    ]
  }
  ```

#### 5. Объекты по IDs
- `POST /api/v1/objects/getByIds`
- Тело: `{ "id": ["s6", "s7"], "type": "sensor" }`
- `type` — опционально, `sensor` или `device` (фильтр по типу)
- Ответ:
  ```json
  {
    "objects": [
      {
        "id": "s6",
        "name": "DHT22 Temperature",
        "type": "sensor",
        "topic": "sensors/dht22",
        "value": "24.40",
        "spec": { "model": "dht22", "unit": "℃", "minorPart": 2 }
      }
    ]
  }
  ```

#### 6. Команда устройству
- `POST /api/v1/objects/command/:deviceId`
- Тело: `{ "value": "1" }`
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
├── data/                         # Общие конфиги (unitId1.config.ts — объекты юнита unitId1 для ObjectsService и UnitsService)
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
