[↑ SASPIK Cluster App](../README.md)

# SASPIK Cluster Server

##### ⧫ Серверное приложение, реализующее REST API для работы с Юнитами, Объектами и сценариями автоматизации

Юниты, объекты и правила хранятся в MongoDB (коллекции `units`, `objects`, `rules`); последние значения объектов читаются из InfluxDB (данные поступают через Telegraf из MQTT-брокера).
Построено на Express.js с Dependency Injection (InversifyJS).

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

| Метод | Путь | Описание |
|---|---|---|
| `GET` | `/health` | Health check |
| `GET` | `/api/getTimestamp` | Текущее время сервера |
| `GET` | `/api/v1/units/list` | Список юнитов с вложенными объектами и правилами (из MongoDB) |
| `GET` | `/api/v1/units/:id` | Юнит по ID (с вложенными объектами и правилами) |
| `POST` | `/api/v1/units` | Создание юнита: `{ id, name, description? }` |
| `PATCH` | `/api/v1/units/:id` | Обновление юнита: `{ name, description? }` (id не изменяем) |
| `DELETE` | `/api/v1/units/:id` | Удаление юнита вместе с его объектами (каскадно; правила не затрагиваются) |
| `POST` | `/api/v1/objects/list/:type` | Объекты по типу (`sensor`/`device`). `value` из InfluxDB |
| `POST` | `/api/v1/objects/getByIds` | Объекты по IDs |
| `POST` | `/api/v1/objects/command/:deviceId` | Команда устройству |
| `POST` | `/api/v1/objects/getLastSensorsData` | Последние показания сенсоров |
| `POST` | `/api/v1/objects` | Создание объекта: `{ id, name, type, spec, description?, unitId }`, топик формируется сервером по паттерну `` `${type}/${unitId}/${id}` `` |
| `PATCH` | `/api/v1/objects/:id` | Обновление объекта: `{ name, type, spec, description?, unitId }` (id и топик пересчитываются сервером) |
| `DELETE` | `/api/v1/objects/:id` | Удаление объекта: `{ unitId }` |
| `GET` | `/api/v1/rules` | Список правил (формат rule-engine, из MongoDB) |
| `POST` | `/api/v1/rules` | Upsert правила (создание/обновление по `id`) |
| `PATCH` | `/api/v1/rules/:id` | Включить/отключить правило (`{ "enabled": true/false }`) |
| `DELETE` | `/api/v1/rules/:id` | Удалить правило |
| `POST` | `/api/v1/mqtt/publish` | Публикация MQTT-сообщения |
| `POST` | `/api/v1/mqtt/subscribe` | Подписка на MQTT-топик |
| `POST` | `/api/v1/mqtt/unsubscribe` | Отписка от MQTT-топика |

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
