# AGENTS.md — правила работы с монорепозиторием САСПИК

Файл для ИИ-агентов и новых участников: что за проект, как собирать, какие контракты нельзя ломать, куда идти с типовой задачей. Компактный; детали — по ссылкам.

## Как пользоваться

Порядок источников истины (при расхождении — выше побеждает):

1. **Код и конфиги** (`docker-compose.yml`, `platformio.ini`, `package.json`, `config.h`, `mosquitto.acl`, `telegraf.conf`).
2. **Журналы `IN_PROGRESS.md`** в подпроектах — актуальнее обзорных документов: фиксируют последние решения и инциденты.
3. **Этот файл** — выжимка правил и контрактов для агента.
4. `PROJECT_CONTEXT.md` и `DETAILS.md` — обзор репозитория для человека (сверены с кодом 2026-11-06); при расхождении всё равно доверять коду и журналам.

Типичный маршрут агента: этот файл → нужный `IN_PROGRESS.md` → `saspik-cluster/docs/*` → код.

## Что за проект

САСПИК — программно-аппаратная платформа для совместного производства: IoT-устройства → MQTT-кластер → веб-UI.

Иерархия: **Кластер** (автономный узел, docker-стек) → **Юнит** (функциональная зона: теплица, цех, склад) → **Объект** (устройство: `sensor` или `device`).

Четыре подпроекта:

| Подпроект | Что это | Состояние |
| :--- | :--- | :--- |
| `saspik-cluster` | Ядро: Express API, React UI, Mosquitto, rule-engine, InfluxDB/Telegraf | Работает, пилот на проде |
| `saspik-iot` | Прошивки ESP32 (PlatformIO/Arduino): шлюз, сенсоры, автоматика | Развивается, host-тесты есть |
| `saspik-greenhouse` | Пилот «автономная теплица» | Только документация/планирование |
| `saspik-meshrover` | Рой роботов (UWB, PlatformIO + ESP-IDF) | Pre-MVP, этап 1 — дальномер |

## Правила работы агента

- **Язык**: ответы и документация — по-русски; код, идентификаторы, коммиты — англоязычные.
- **Без заглушек**: не оставлять `TODO`-комментарии и недописанный код. Не реализовано — не пишем вовсе; нет — так и сказать.
- **Минимальный дифф**: править только то, что требует задача; попутный рефакторинг не делать.
- **Компилируемость**: каждый файл должен собираться. Перед ответом — сборка/линт/тест затронутого компонента (см. «Команды»).
- **SOLID и DI**: в backend каждый сервис/контроллер — интерфейс + реализация, привязка в `server/src/index.ts`, токены в `types.ts`.
- **Стиль**: `camelCase` для TS, `snake_case` для C++, файлы/папки React — kebab-case (`unit-form`, `use-objects-list-fetching`).
- **Секреты**: не коммитить `.env`, `mqtt/passwordfile` с реальными паролями, `src/env_config.h` (генерируется `extra_script.py` из `.env`), папку `saspik-private/` (вне git).
- **Контракты не ломать**: топики MQTT, ACL, `telegraf.conf`, формат лог-конверта, схему API — менять только осознанно, с обновлением `saspik-cluster/docs/`.
- **Проверка «есть ли docker/Mongo/брокер»**: без доступного стека проверять только сборку/линт/unit-тесты, а ручные сценарии явно оставлять пользователю.

## Карта репозитория

```
README.md            описание проекта, лендинг-ссылка
PROJECT_CONTEXT.md   обзорная карта репозитория (может отставать)
DETAILS.md           расширение PROJECT_CONTEXT (версии, протоколы)
AGENTS.md            этот файл
docs.html            агрегированный HTML всех README (генерируется)
index.html           промо-лендинг (GitHub Pages)
_Журнал.md           журнал идей/запросов владельца
plans/               рабочие планы (bridge-pattern и пр.)
```

| Путь | Назначение |
| :--- | :--- |
| `saspik-cluster/IN_PROGRESS.md` | Журнал решений и инцидентов кластера |
| `saspik-cluster/docs/` | `API.md`, `mqtt-topics.md`, `logs.md`, `ObjectsService.md`, `UnitsService.md`, `RulesService.md`, `HistoryService.md` |
| `saspik-cluster/saspik-cluster-app/server/` | Express + Inversify: `src/{controllers,services,data-store,config,mqtt}` |
| `…/client/` | React 19 SPA (CRA), hooks + fetch, `mock-api` |
| `…/mqtt-rule-engine/` | Отдельный воркер правил (`conditions/`, `actions/`, `core/engine.ts`) |
| `…/mqtt/` | `mosquitto.conf`, `mosquitto.acl`, `Dockerfile` |
| `…/telegraf/`, `…/influxdb/` | MQTT→Influx, провижининг бакетов и Flux-запросы |
| `…/nginx/` | Статика SPA + прокси `/api` |
| `saspik-iot/IN_PROGRESS.md`, `devices.md` | Журнал IoT и реестр устройств (objectId) |
| `saspik-iot/docs/classification.md` | Классификатор устройств |
| `saspik-iot/saspik-iot-sketches/` | `esp32-boilerplate` (saspik-core + host-тесты), `esp32-local-mqtt`, `esp32-controller`, `esp32-battery-check`, `esp32-node-template`, `saspik-iot-node-sensor-1`, `shared/` |
| `saspik-meshrover/software/` | PlatformIO + ESP-IDF, `lib/dwm3000`, `include/config.h` |
| `saspik-greenhouse/docs/` | `grow_chamber.md`, `sensor-ph-ec.md`, `smeta.md` |
| `saspik-private/` | Вне git: `opencode/` (session_log, skills), `articles/`, `manuals/`, `TODO.md` |

## Команды

### Кластер (основной стек)

```bash
cd saspik-cluster/saspik-cluster-app
cp example.env .env              # заполнить значения
docker compose up --build -d     # backend, nginx, mongo, mosquitto, rule-engine, influxdb, telegraf
curl http://localhost/health     # nginx → 200
curl http://localhost:3001/health
docker compose -f docker-compose.test.yml up   # тест-стек без backend/nginx
```

### Backend

```bash
cd saspik-cluster/saspik-cluster-app/server
npm install
npm run dev      # nodemon + ts-node
npm run build    # tsc → dist/
npm start
npm run lint
```

Тестов нет: `npm test` завершается ошибкой — это нормальное состояние, не «сломанный тест».

### Client

```bash
cd saspik-cluster/saspik-cluster-app/client
npm install
npm start          # CRA dev-server
npm run start:mock # REACT_APP_MOCK_MODE=true, без API
npm run build
npm test           # Jest + RTL
```

### Rule engine

```bash
cd saspik-cluster/saspik-cluster-app/mqtt-rule-engine
npm install
npm run dev        # ts-node src/index.ts
npm run build && npm start
npm run test       # test/test-runner.ts на мок-адаптере (без брокера)
npm run lint
```

### Прошивки ESP32

```bash
cd saspik-iot/saspik-iot-sketches/esp32-boilerplate
pio run -e sensor_esp32     # env: {gateway,sensor,device}_{esp32,s2,s3,c3} + native
pio test -e native          # host-тесты Unity (~29 тестов)
pio run -e gateway_esp32 -t upload

cd ../esp32-local-mqtt
pio run                     # сборка; pio run -t upload; pio device monitor
```

`upload_speed = 115200` (CP2102), кастомные `partitions.csv` 4MB с разделом `spiffs`.

### MeshRover

```bash
cd saspik-meshrover/software
pio run -e esp32-s3-devkitc-1   # framework = espidf; альтернатива — idf.py build при заданном IDF_PATH
```

## Контракты

### MQTT

Структура топика: `{домен}/{unitId}/{objectId}[/суффикс]`. Источник истины — `saspik-cluster/docs/mqtt-topics.md`.

| Топик | Направление | Payload |
| :--- | :--- | :--- |
| `sensor/{unitId}/{objectId}` | device → broker | JSON-карта по ключам каналов (`{"температура":"23.4","влажность":"41"}`) |
| `device/{unitId}/{objectId}/command` | broker → device | `{"channel":"свет","value":"ON"\|"OFF"}` |
| `device/{unitId}/{objectId}/state` | device → broker, retained | JSON-карта всех каналов: `{"свет":"ON","полив":"OFF"}` |
| `device/{unitId}/{objectId}/status` | device → broker, retained + LWT | `{"status":"online"}` / `{"status":"offline"}` |
| `device/{unitId}/{objectId}/log` | device → broker | лог-конверт |
| `units/{unitId}/commands/a_relay1..4` | rule-engine → broker | `{"state":"1"\|"0"}` — климат-контроль |
| `units/{unitId}/sensors` | агрегаты по юниту | — |
| `server/+/log`, `rule-engine/+/log` | сервисы → broker | лог-конверт |
| `healthcheck/ping` | healthcheck mosquitto | `{"payload":"test"}` — обязан быть валидным JSON |

Правила:

- `{objectId}` в топике — **база без постфикса**: для объекта `saspik-awqm001-device` топики строятся от `saspik-awqm001`.
- Новая схема команд — **один `/command` на объект**, имя канала в payload; имена каналов произвольные («свет», «полив»).
- **Legacy**: объекты с id `saspik.sa.*` (например `saspik.sa.wm.m003`) команду принимают в базовом топике `device/{unitId}/{objectId}` с `{"state":"ON"|"OFF"}` — их прошивка `/command` не знает. Старые `units/.../commands/*` — `1`/`0`.
- Лог-конверт един для всех компонентов: `{level,src,event,msg,topic,unitId,objectId,uptime,cause}`.
- ACL (`mqtt/mosquitto.acl`) default-deny: `sensor/#`, `device/#`, `units/#`, `led/#`, `server/+/log`, `rule-engine/+/log`, `healthcheck/#`, `clients/%c/#`. Новый домен топиков без строки ACL молча отбрасывается.
- Telegraf: данные — `sensor/#`, `units/#`, `led/#`, `device/+/+/state`; логи — `device/+/+/log`, `server/+/log`, `rule-engine/+/log`; `json_string_fields = ["*"]` для данных (имена каналов произвольные).

### HTTP API

Префикс `/api/v1`, смонтированы только `/objects`, `/units`, `/rules`. Вне префикса: `GET /health`, `GET /api/getTimestamp`.

- Units: `GET /units/list`, `GET /units/:id`, `POST /units`, `PATCH /units/:id`, `DELETE /units/:id` (каскадно удаляет объекты).
- Objects: `POST /objects/list/:type`, `POST /objects/getByIds`, `POST /objects/command/:deviceId`, `POST /objects`, `PATCH /objects/:id`, `DELETE /objects/:id`; `POST /objects/getLastSensorsData` — заглушка `{}`.
- Rules: `GET /rules`, `POST /rules` (upsert по `id`), `PATCH /rules/:id`, `DELETE /rules/:id`.
- `MqttController` реализован, но **не смонтирован**; `GET /history/list` не существует.
- Ошибки: `{error: string}` + HTTP-код; централизованной валидации DTO нет (`validate.middleware.ts` не подключён).

### Данные

- MongoDB: коллекции `units`, `objects`, `rules`; объект привязан к юниту полем `unitId`.
- InfluxDB: бакет `mqtt` (данные, без retention) и `logs` (7 дней). Сервер читает последние значения Flux-запросом `range(-30d) → filter(topic) → last() → group() → sort → limit(1)`; `group()` обязателен.
- Климат-контроль живёт в rule-engine (правила `server/data/rules.json` с подстановкой `${CLIMATE_*}` и `${expr:...}`); серверный `ClimateControlService` отключён.

## Инварианты

1. Топик объекта = `{type}/{unitId}/{base}` без постфикса — иначе теряются данные и команды.
2. `updateObject` пересчитывает топик только для канонических `sensor|device/{unitId}/...`; произвольные seed-топики сохраняются — иначе правка через UI рвёт подписки.
3. Сиды идемпотентны: `units`/`objects` — только в пустые коллекции, `rules` — аддитивно по `id` (существующие не перезаписываются). В `server/src/data/` засидены `unitId1` и `unitId2`; новые объекты (`saspik-awqm001` — `unitId3`, `saspik-aenm001` — `unitId2`) живут в Mongo, а не в сиде.
4. Правило с `enabled: false` движок пропускает и не подписывается на его топики.
5. `[agent] hostname = "telegraf"` в `telegraf.conf` менять нельзя: смена плодит серии в Influx и «статику» значений в UI (инцидент 2026-09-20).
6. Любой не-JSON payload в топике, который слушает telegraf, роняет весь батч `mqtt_consumer` — поэтому `healthcheck/ping` валиден и исключён из входов.
7. ACL вшит в образ `mosquitto` (`mqtt/Dockerfile`): после правки ACL нужен `docker compose up -d --build mosquitto`, а не `restart`.
8. Retained `device/.../state` и `.../status` — единственный источник состояния устройства для UI; `state` — карта всех каналов, имена совпадают со `spec.key`.
9. Климат-правила дублируются на каждый `unitId`: движок не подставляет `unitId` из wildcard-топика.
10. Сервер не публикует команду с `channel`, которого нет в `device.spec` — неизвестный ключ игнорируется.
11. ESP32: MQTT-connect неблокирующий (таймауты + `yield()`), иначе Task WDT → `rst:0x8`; состояние автоматов восстанавливается из retained.
12. ESP-NOW v2 (boilerplate) несовместим с v1 (`saspik-iot-node-sensor-1`): другой формат кадра и CRC-8/MAXIM.
13. Device публикует только события (startup/reboot/reconnect/connect), а не каждое чтение датчика — анти-спам в Influx.

## Типовые задачи

| Задача | Куда | Что учесть |
| :--- | :--- | :--- |
| Новый эндпоинт API | `server/src/controllers/` + `const.ts` + DI в `index.ts` | Роуты монтируются в `app.ts`; ошибки — `{error}` + код |
| Новый экран/форма в UI | `client/src/components/`, `hooks/`, `mock-api` | В мок-режиме свои данные; контролы устройства — по `spec` |
| Новый канал у устройства | `config.h`: `CHANNELS[]` (прошивка) + `spec` объекта в UI | Ключ канала одинаков в прошивке и `spec.key`; `state` — карта всех каналов |
| Новый MQTT-домен | `mqtt/mosquitto.acl` + `telegraf/telegraf.conf` + `docs/mqtt-topics.md` | Default-deny; JSON в telegraf; пересборка образа mosquitto |
| Правило автоматизации | `server/data/rules.json` (сид) и/или Mongo | Дублировать на unitId; `${VAR}`/`${expr:...}` |
| Новое условие/действие rule-engine | `mqtt-rule-engine/src/{conditions,actions}` + `constants.ts` + `config/builder.ts` | Тест в `test/test-runner.ts` на мок-адаптере |
| Новая прошивка ESP32 | Копия `esp32-boilerplate` или скетч по шаблону | objectId — в `devices.md`; топики — в `docs/mqtt-topics.md` |
| Показания «застыли» в UI | `influxDbStateStore.service.ts`, `telegraf.conf`, бакеты | Проверять `hostname`, серии, `group()` перед `last()` |
| Отладка логов устройства | `docs/logs.md`, бакет `logs`, `influxdb/queries/logs-view.flux` | `_time` — граница окна агрегации, не время события |
| Миграция Mongo | Прямые команды в Mongo | `SeedService` не перезапишет существующие коллекции |

## Грабли и известные проблемы

- `esp32-local-mqtt`: перегрузка `publish(const char*, const char*, boolean)` публикует данные как **retained** — для сенсора и логов нужен явный `publish(topic, (const uint8_t*)buf, len, false)` (зафиксировано в `IN_PROGRESS.md`, не исправлено).
- `esp32-local-mqtt`: результат `subscribe()` не проверяется — при отказе устройство молча остаётся без команд.
- Backend без автотестов; e2e-проверок с Mongo/Influx нет — регрессии ловятся вручную.
- `ClimateControlService` отключён (вызов в `app.ts` закомментирован), но `CLIMATE_*` env используются правилами rule-engine — дублирование логики.
- Rightech-интеграция (`RIGHTECH_API_TOKEN`, `MQTT_BROCKER_API_URL`) — мёртвый код.
- Скиллы и правила агента — в `.dsh/skills/` и `SKILLS.md` (в git); в `saspik-private/opencode/` остаётся приватная копия скиллов и `session_log.md`. Если приватной папки нет — работать по `.dsh/skills/`.
- Прод-инфраструктура (брокер `185.72.145.19`) обслуживается вручную; способ деплоя и миграций вне репозитория.
- `npm run lint` сервера может падать на `Object.hasOwn` из-за старой связки Node/ESLint — не связано с правками.

## Соглашения

- **Нейминг устройств**: `saspik-{роли}{протокол}{питание}{nnn}` — роли `g`/`s`/`a`, протокол `wq`/`en`/`lr`, питание `m`/`b`, шаблон `nnn=000`. Реестр занятых id — `saspik-iot/devices.md` (намеренно вне `docs/`, чтобы не краулился генератор docs).
- **Фактические id**: `saspik-awqm001` (`unitId3`, local-mqtt, автоматы «свет»/«полив»), `saspik-aenm001` (`unitId2`, boilerplate). Legacy `saspik.sa.wm.m003` работает на своей прошивке и не переименовывается.
- **README подпроектов**: заголовок `# ⧫ Название`, строка-описание, ссылка `[↑ Назад](../README.md)`.
- **`IN_PROGRESS.md`**: единый список задач, сверху незавершённые (`В работе` → `На доработку` → `Нужно взять в работу`), ниже `Выполнено`; даты не пишутся, статусы — в записи.
- **Коммиты**: Conventional Commits — `feat|fix|style|docs|refactor|chore(scope): описание`, scope — `cluster`, `iot`, `meshrover`, `greenhouse` (можно перечисление: `fix(cluster,iot): …`).
- **Логи**: единый JSON-конверт во всех компонентах кластера.
- **Документы**: при изменении контрактов (топики, API, env) обновлять `saspik-cluster/docs/`; `docs.html` перегенерировать скиллом `docs-gen` (`.dsh/skills/docs-gen/`).

## Чего не делать

- Не менять `hostname` telegraf, формат топиков и ACL «по ходу» задачи.
- Не пересчитывать неканонические топики при обновлении объекта.
- Не удалять legacy-объекты `m001`/`m003` и их сиды.
- Не менять `unitId`/`objectId` устройства без явного решения владельца (есть история рассинхрона прошивки и конфигов).
- Не коммитить секреты и не печатать содержимое токенов в логах/ответах.
- Не добавлять зависимости, если задача решается стандартными средствами.
- Не «чинить» отсутствующие тесты backend и не считать `npm test` сервера показателем.

## Скиллы

- Проектные скиллы живут в `.dsh/skills/<name>/SKILL.md` — DSH подхватывает их автоматически; индекс и фразы-триггеры — [`SKILLS.md`](SKILLS.md).
- Состав: `add-task` (TODO.md), `in-progress` (IN_PROGRESS.md), `project-status` (сводка по проектам), `session-log` (журнал сессий), `docs-template` + `docs-gen` (оформление и сборка `docs.html`), `firmware-gen` (новый скетч), `schematic-gen` (схемы и диаграммы), `md-render` (показ markdown).
- Рабочая версия правил — в `.dsh/skills/`; копия в `saspik-private/opencode/skills/` не синхронизируется автоматически, правь рабочую.
- `TODO.md`, `IN_PROGRESS.md` подпроектов — в публичном git; `session_log.md` живёт в `saspik-private` (вне публичного git), его правки не коммитятся.

## Открытые вопросы

Не решать самовольно — уточнять у владельца:

1. Судьба `ClimateControlService`: остаётся отключённым навсегда (климат только в rule-engine) или включается?
2. Rightech API — выпиливать или планируется к использованию?
3. Процедура прод-деплоя и миграций Mongo (Portainer? ручные команды?).
4. Кластер-кластерное взаимодействие задекларировано в README, кода нет — когда и в каком виде?
5. Нужен ли объект `unitId3`/`saspik-awqm001` в сиде кластера или он создаётся только через UI?

## Поддержка этого файла

- Меняешь топики, ACL, API, env, схему данных или правила сборки — правишь `AGENTS.md` в том же изменении.
- Держи объём в пределах ~350 строк: детали уходят в `PROJECT_CONTEXT.md`, `DETAILS.md` и `docs/`.
- `PROJECT_CONTEXT.md`/`DETAILS.md` обновлять по отдельному запросу; при противоречии с кодом доверять коду и журналам, а расхождение — сообщать владельцу.
