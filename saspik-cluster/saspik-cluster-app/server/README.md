[↑ Saspik-cluster](../../README.md)

# ⧫ Saspik-cluster — Бекенд

#### Серверное приложение, реализующее REST API для работы с Юнитами, Объектами и сценариями автоматизации

Юниты, объекты и правила хранятся в MongoDB (коллекции `units`, `objects`, `rules`)

Последние значения объектов читаются из InfluxDB (данные поступают через Telegraf из MQTT-брокера)

Построено на Express.js с Dependency Injection (InversifyJS)

### Основные компоненты:
- **Контроллеры** — обработка HTTP запросов
- **Сервисы** — бизнес-логика
- **Data Store** — слой доступа к MongoDB (`MongoService`, репозитории `UnitsRepository`/`ObjectsRepository`/`RulesRepository`, `SeedService`)
- **State Store** — хранилище последних значений объектов в InfluxDB (`InfluxDbStateStoreService`, интерфейс `IStateStoreService`)
- **MQTT** — `LocalMqttService`: подключение к локальному брокеру (`MQTT_BROKER_URL`, авторизация `MQTT_USERNAME`/`MQTT_PASSWORD`), автопереподписка при реконнекте. `MqttService` — фасад над ним для остальных сервисов
- **Контроль климата** — `ClimateControlService`: гистерезисные контуры температуры и влажности с дневным/ночным профилями. Подписывается на `units/{unitId}/sensors`, команды публикует в `units/{unitId}/commands/a_relay1..4` (свет, увлажнитель, вентилятор, вода). Цикл управления в настоящий момент отключён: вызов `executeControll()` закомментирован в `app.ts`, читается только `CLIMATE_CONTROL_UNIT_ID`
- **DTO** — объекты передачи данных

### Технические детали:
- **TypeScript**, Express.js, порт 3001
- **Dependency Injection** (InversifyJS)
- **MongoDB** (официальный драйвер `mongodb`), class-validator, tslog

### Запуск

```bash
npm install
npm run dev           # nodemon + ts-node (разработка)
npm run build         # компиляция tsc в dist/
npm start             # production: node ./dist/index.js
npm run lint          # ESLint
npm run lint:fix      # ESLint с автофиксом
```

### API

[Подробнее об API можно прочитать здесь →](../../docs/API.md)

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

> `MqttController` (publish/subscribe/unsubscribe) реализован, но не смонтирован в роутере — эндпоинты недоступны.

### Переменные окружения

| Переменная | Описание | По умолчанию |
|---|---|---|
| `PORT` | Порт HTTP-сервера | `3001` |
| `RIGHTECH_API_TOKEN` | Токен Rightech API | — |
| `MONGODB_URL` | Строка подключения к MongoDB | — |
| `INFLUXDB_URL` | URL InfluxDB (State Store) | — |
| `INFLUXDB_TOKEN` | Токен InfluxDB | — |
| `INFLUXDB_ORG` | Организация InfluxDB | — |
| `INFLUXDB_BUCKET` | Бакет InfluxDB | — |
| `INIT_RULES_SEED_PATH` | Файл первичного наполнения коллекции `rules` | — |
| `MQTT_BROKER_URL` | URL локального MQTT-брокера | `mqtt://localhost:1883` |
| `MQTT_USERNAME` / `MQTT_PASSWORD` | Авторизация на брокере | — |
| `CLIMATE_CONTROL_UNIT_ID` | Юнит, для которого запускается контроль климата (цикл сейчас отключён) | — |
| `CLIMATE_T_SET` / `CLIMATE_T_HYST` | Целевая температура и гистерезис, °C | `25` / `1` |
| `CLIMATE_T_MAX` / `CLIMATE_T_MIN` | Аварийные пороги вентиляции, °C | `27` / `23` |
| `CLIMATE_RH_SET` / `CLIMATE_RH_HYST` | Целевая влажность и гистерезис, % | `70` / `10` |
| `CLIMATE_NIGHT_T_SET` / `NIGHT_T_HYST` | Ночная температура и гистерезис, °C | `22` / как днём |
| `CLIMATE_NIGHT_T_MAX` / `NIGHT_T_MIN` | Ночные аварийные пороги, °C | `24` / `20` |
| `CLIMATE_NIGHT_RH_SET` / `NIGHT_RH_HYST` | Ночная влажность и гистерезис, % | `60` / как днём |
| `CLIMATE_NIGHT_START_HOUR` / `NIGHT_END_HOUR` | Границы ночного режима, ч | `22` / `8` |
| `MOSCOW_OFFSET_HOUR` | Часовое смещение при расчёте ночи | `3` |
| `CLIMATE_CONTROL_INTERVAL_MS` | Период цикла управления, мс | `30000` |

Файл `.env` загружается из корня репозитория по абсолютному пути (кроме `NODE_ENV=prod`).

При первом запуске (пустая БД) `SeedService` заполняет коллекции `units`, `objects`, `rules` из `src/data/*.config.ts` и файла `INIT_RULES_SEED_PATH` (env-подстановка `${VAR}` и `${expr:...}` выполняется до записи в БД). Сид идемпотентен: непустые коллекции не перезаполняются.
