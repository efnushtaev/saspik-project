# DETAILS.md

Расширение к [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md): версии, точные контракты, протоколы, история. Ссылки из основного документа вида → DETAILS#раздел ведут сюда.

- Сверено с кодом и журналами 2026-11-06.
- Формат журналов `IN_PROGRESS.md` изменён: задачи без дат, сверху незавершённые; исторические записи ниже сохраняют упоминания дат внутри текста.

## Стек: полные версии

### server (`saspik-cluster/saspik-cluster-app/server/package.json`)

- deps: express ^4.19.2, inversify ^6.0.2, mongodb ^6.10.0, mqtt ^5.0.0, @influxdata/influxdb-client ^1.35.0, axios ^1.4.0, body-parser ^1.20.2, class-transformer ^0.5.1, class-validator ^0.14.1, cors ^2.8.5, dotenv ^17.2.3, reflect-metadata ^0.2.2, tslog ^4.9.3.
- devDeps: typescript ^5.5.4, ts-node ^10.9.2, nodemon ^3.1.4, eslint ^9.7.0, prettier ^3.3.3, @typescript-eslint/* ^8.26.0, eslint-plugin-import ^2.31.0, eslint-config-prettier ^9.1.0, eslint-plugin-prettier ^5.2.1.
- Node-версия образа: `node:18-alpine` (multi-stage `server/Dockerfile`).
- Скрипты: `dev`, `dev:inspect`, `build`, `start`, `lint`, `lint:fix`, `test` (заглушка → exit 1).

### client (`client/package.json`)

- react ^19.1.0, react-dom ^19.1.0, react-router-dom ^6.30.3, react-scripts 5.0.1, typescript ^4.9.5, bem-react-classname ^1.3.2, @ant-design/icons ^6.1.0, web-vitals ^2.1.4, postcss-selector-parser ^6.1.2.
- Тесты: @testing-library/react ^16.3.0, jest-dom ^6.6.3, user-event ^13.5.0, @testing-library/dom ^10.4.0.
- engines: node >= 18. Скрипты: `start`, `start:mock`, `build`, `test`, `eject`.

### mqtt-rule-engine (`mqtt-rule-engine/package.json`)

- mqtt 5.3.5, mongodb ^6.10.0; dev: typescript ^5.7.3, ts-node, eslint ^9.17.0, @types/node ^22.10.6.
- engines: node >= 18. Скрипты: `dev`, `build`, `start` (`node dist/src/index.js`), `test` (`ts-node test/test-runner.ts`), `lint`, `clean`.

### Прошивки

- `esp32-local-mqtt/platformio.ini`: platform espressif32, framework arduino, `monitor_speed=115200`, `upload_speed=115200`, `board_build.filesystem=littlefs`, кастомная `partitions.csv` (4MB, раздел назван `spiffs`); libs: PubSubClient ^2.8, ArduinoJson ^6.21.3, DHT ^1.4.4, OneWire ^2.3.8, `lib_extra_dirs = ../shared` (wifi-config, device-log).
- `esp32-boilerplate/platformio.ini`: общая секция `[env]` (esp32 platform, arduino, `-Iinclude`, 115200, littlefs, partitions, `lib_extra_dirs = ../shared`), env `{gateway,sensor,device}_{esp32,s2,s3,c3}` + `native` (host-тесты Unity, `lib_ldf_mode = deep+`), роль — build-флаг `-DDEVICE_ROLE=1|2|3`.
- `saspik-meshrover/software/platformio.ini`: `platform = espressif32`, `framework = espidf`, `board = esp32-s3-devkitc-1`, `build_flags = -Iinclude -DCONFIG_UWB_SS_TWR -DCONFIG_UWB_CHANNEL=5 -DCONFIG_UWB_PRF=1`, `core_dir = .pio`.

## HTTP API: точные роуты

### Монтирование (`server/src/const.ts`, `server/src/app.ts`)

- Префикс `/api/v1`; домены: `/objects`, `/units`, `/rules`; `ControllersDomens.MQTT` объявлен, но роутер не смонтирован.
- Вне префикса: `GET /health` → `200 OK` (текст), `GET /api/getTimestamp` → `{timestamp}`.

### UnitsController (`server/src/controllers/units.controller.ts`)

- `GET /units/list` → юниты с objects+rules.
- `GET /units/:id` → юнит; 404 если нет.
- `POST /units` → `{id, name, description?}`.
- `PATCH /units/:id` → `{name, description?}` (id не меняется).
- `DELETE /units/:id` → каскадно удаляет объекты юнита (`objectsRepository.deleteByUnitId` + удаление юнита); правила не трогаются.

### ObjectsController

- `POST /objects/list/:type` (`sensor|device`) + body `{unitId?}` → объекты, обогащённые значениями каналов.
- `POST /objects/getByIds` body `{id[], type?, unitId?}`.
- `POST /objects/command/:deviceId` body `{value, key?, unitId?}`:
  - канонический `device/...` + известный `key` + id не `saspik.sa.*` → publish в `${topic}/command` c `{"channel": key, "value": value}`;
  - неизвестный `key` → лог и **без публикации**;
  - legacy (`units/...`, объекты `saspik.sa.*`, без `key`) → publish в базовый `topic` c `{specKey: value}` (specKey = `spec[0].key` или `"state"`).
  - ответ `{success: true}`; при исключении — 500 `{success: false, ...}`.
- `POST /objects/getLastSensorsData` — заглушка, возвращает `{}`.
- `POST /objects` → создание: схлопывает постфикс `-sensor`/`-device`, дописывает постфикс по типу, строит topic `${type}/${unitId}/${base}`.
- `PATCH /objects/:id` → обновление; topic пересчитывается только для канонических `sensor|device/{unitId}/...`, иначе сохраняется.
- `DELETE /objects/:id` body `{unitId}`.

Обогащение (`ObjectsController.enrichObject`): каждый `spec` превращается в `{key, value, spec}`, `value` — из state store по `object.topic` и `spec.key` (округление по `minorPart`), `updatedAt` — max timestamp каналов, `status` — из подписки `device/+/+/status`.

### RulesController

- `GET /rules?unitId=...` → правила; движок сам фильтрует `enabled:false`.
- `POST /rules` → upsert по `id`.
- `PATCH /rules/:id` → частичное обновление (`{enabled: false}` — мягкое отключение).
- `DELETE /rules/:id`.

### Что не реализовано

- `MqttController` (publish/subscribe) — код есть, роуты не монтируются.
- `GET /history/list` — нет контроллера/сервиса; раздел [`docs/HistoryService.md`](saspik-cluster/docs/HistoryService.md) помечен «запланировано».
- `validate.middleware.ts` есть, но к роутам не подключён: валидация DTO отсутствует.

## MQTT: детальные контракты

### Домены

| Домен | Направление | Пример |
| --- | --- | --- |
| `sensor/{unitId}/{objectId}` | device → broker | `sensor/unitId3/saspik-awqm001` |
| `device/{unitId}/{objectId}/command` | broker → device | `device/unitId3/saspik-awqm001/command` |
| `device/{unitId}/{objectId}/state` | device → broker (retained) | `{"свет":"ON","полив":"OFF"}` |
| `device/{unitId}/{objectId}/status` | device → broker (retained) + LWT | `{"status":"online"}` / `{"status":"offline"}` |
| `device/{unitId}/{objectId}/log` | device → broker | лог-конверт |
| `units/{unitId}/commands/a_relay1..4` | rule-engine → broker | `{"state":"1"}` |
| `units/{unitId}/sensors` | агрегированные данные | — |
| `server/worker/log`, `rule-engine/worker/log` | логи | лог-конверт |
| `healthcheck/ping` | healthcheck mosquitto | `{"payload":"test"}` |

### Команды устройств vs климата

- `device/.../command` — `{"channel":"<spec.key>","value":"ON"|"OFF"}`; `value` принимает также `1`/`0`/`TRUE`/`FALSE`; неизвестный `channel` устройство игнорирует; legacy `{"state":"ON"}` без `channel` применяется к первому автомату.
- Legacy-объекты `saspik.sa.*` (например `m003`) — команда в базовом топике, payload `{"state":"ON"|"OFF"}`.
- `units/{unitId}/commands/a_relay*` — `{"state":"1"|"0"}` (числа строками), см. `_meta.commands` в `rules.json`. Форматы не взаимозаменяемы.

### Состояние и статус

- `state` — retained JSON-карта **всех** каналов, ключи = `spec.key`; публикуется при подключении и каждой смене.
- `status` — retained online + LWT offline; сервер подписан на `device/+/+/status` (с ретраем подписки 3 с) и хранит статусы в памяти для UI.

### Постфиксы и база идентификатора

- Объект в БД: `{base}-sensor` или `{base}-device`; в топике — всегда база без постфикса.
- Прошивка задаёт `OBJECT_ID` базой; сервер при создании дописывает постфикс по типу.
- Legacy-объекты без постфикса совместимы (топик выводится от того же id).

### Логи

- Единый конверт: `{level, src, event, msg, topic, unitId, objectId, uptime, cause}`; поля `unitId/objectId/uptime/cause` — для устройств.
- События device: `startup` (с `cause` из NVS), `reboot`, `connect`, `reconnect` (с `downSec`/`errors`), `diag`; публикация лога — только по событиям, не на каждое чтение.
- rule-engine: `rule-fired`, `rule-error`; server: `connect`, `disconnect`, `mqtt-error`.

## ACL по правилам

`mqtt/mosquitto.acl` (default-deny):

- `healthcheck/#` — readwrite (healthcheck контейнера).
- `clients/%c/#` — namespace по Client ID.
- `sensor/#`, `device/#`, `units/#`, `led/#` — данные, команды, состояния, логи устройств.
- `server/+/log`, `rule-engine/+/log` — лог-топики сервисов.

Нюансы:

- `device/#` покрывает команды, `/state`, `/status` и `/log`; ранее было `device/+/+/log` — команды и состояния молча отбрасывались (инцидент и фикс `a9f4b43`).
- ACL вшит в образ (`mqtt/Dockerfile`): после правки нужен `--build`, иначе работает старый ACL.
- mosquitto отдаёт SUBACK `0` даже на запрещённые топики (фильтрация на доставке) — проверять ACL по факту прохождения сообщений.
- `allow_anonymous false`, `password_file /mosquitto/config/passwordfile`, `persistence true`, `log_type notice`.

## Правила климат-контроля

Сид: [`server/data/rules.json`](saspik-cluster/saspik-cluster-app/server/data/rules.json).

- `_meta`: `description`, `data_sources`, `commands`, `temperature_zones`, `@todo` (дублирование на unitId).
- Правила (11): климат `temp_emergency_high`, `temp_emergency_low`, `fan_on/off`, `humidifier_on/off`, `light_on/off`, `water_on`; автоматизация `unitId3`: `m003_led_on_temp_high`, `m003_led_off_temp_low` (триггер `sensor/unitId3/saspik.sa.wm.m003`, команда в legacy-топик).
- Подстановка: `${CLIMATE_*}` / `${NIGHT_*}` / `${MOSCOW_OFFSET_HOUR}` / `${WATER_TIMEOUT_MS}`; выражения `${expr:...}` вычисляются безопасным парсером (без `eval`) в `seed.service.ts` и `mqtt-rule-engine/src/config/loader.ts`.
- Условия: `jsonpath`, `timeBetween` (поле `$.timestamp`), композиты `and/or/not`, `topicRegex`, `payloadEquals`.
- Действия: `publish` (в `units/.../commands/a_relay*`), `log`, `timeout` (отложенное выключение полива по `WATER_TIMEOUT_MS`).
- Сид **аддитивный**: `seedRules` добавляет только правила, которых нет по `id` (существующие не перезаписываются); `enabled:true` ставится при вставке; `unitId` вычисляется из топика триггера (`^(sensor|units)/([^/]+)/`).
- Ограничение `_meta/@todo`: правила надо дублировать на каждый unitId — движок не подставляет unitId из wildcard-топика.

## Rule Engine: устройство

- Вход: [`src/index.ts`](saspik-cluster/saspik-cluster-app/mqtt-rule-engine/src/index.ts) — `MqttAdapter` + `RuleEngine` + `createRulesProvider()`.
- Провайдеры (`src/providers/rulesProviderFactory.ts`): `mongo` (по умолчанию; требует `MONGODB_URL`), `api` (требует `RULES_API_URL`), `file` (`CONFIG_PATH`, watcher). Опрос: `RULES_POLL_INTERVAL_MS` (5000).
- Ядро `RuleEngine` (`src/core/engine.ts`): индекс `Map<topic, Rule[]>`, exact + wildcard-матчинг, переподписка при перезагрузке правил, без логов на каждое сообщение.
- Контекст: `MessageContext.fromMqttPacket` — JSON-парсинг payload, JSONPath-доступ.
- Условия: `topicRegex`, `jsonpath`, `payloadEquals`, `timeBetween` (смещение из `MOSCOW_OFFSET_HOUR`, по умолчанию 3), композиты `and/or/not`.
- Действия: `publish` (плейсхолдеры `{{field}}`, `retain`), `log` (в `rule-engine/worker/log`), `timeout` (отложенное выполнение).
- Регистрация новых условий/действий: класс + `constants.ts` + `config/builder.ts`.
- Тесты: `npm run test` → `test/test-runner.ts` + `mock-mqtt-adapter.ts` + `sample-rules.json` (без брокера).

## Прошивки: протоколы

### ESP-NOW v2 (esp32-boilerplate)

- Кадр: `[type(1)] [timestamp LE(4)] [fieldId(1)+len(1)+data]* [crc8(1)]`; типы 1=DATA, 2=LOG, 3=COMMAND, 4=ACK.
- Служебные fieldId: `0xFF` — JSON лога, `0xFE` — JSON команды; CRC-8/MAXIM (полином 0x8C, init 0x00); лимит пакета 250 байт.
- Несовместим с v1 (`saspik-iot-node-sensor-1`): другой формат кадра и CRC.
- Модули `saspik-core`: config (NVS, namespace `saspik_app`), portal (captive portal + `wifi-config`), mqtt, espnow, bridge, log (UART+MQTT/ESP-NOW), sys (LightSleep), interfaces (bus I2C/OneWire/SPI/UART, sensor/actuator).
- Каналы: `ChannelBank` собирается из `CHANNELS[]`; Device шлёт ESP-NOW DATA/STATE JSON, шлюз публикует DATA в `sensor/...`, STATE — retained в `device/.../state`, команды маршрутизирует на `device/{unitId}/+/command`; legacy float-поля и `{"state":"ON"}` сохранены.
- Host-тесты: `test/test_bridge`, `test/test_config_manager`, `test/test_timer_registry`, `test/test_topic_builder`, `test/test_esp_now_codec` + `test/mocks/`.

### esp32-local-mqtt (действующее устройство)

- `config.h`: `OBJECT_ID="saspik-awqm001"`, `UNIT_ID="unitId3"`, `OBJECT_TYPE="sensor"`; `CHANNELS[]` — «свет» (GPIO 13, Relay), «полив» (GPIO 26, Relay), `activeLow=false`, `mock=false`; `MAX_CHANNELS=8`.
- Кнопка портала: GPIO 25 (INPUT_PULLUP, нажатие = LOW).
- Брокер: `192.168.0.118:1883`, учётка `admin` (дефолт из примера); таймауты: MQTT-ребут 15 мин, WiFi-ребут 5 мин, WiFi-reconnect 10 с, MQTT-reconnect 3 с, connect-timeout 4 с; интервал датчиков 2 с; DHT22.
- Топики: `device/unitId3/saspik-awqm001/command|state|status|log`.
- Логика: авто-восстановление WiFi в `loop()`, LWT offline, retained online, retained `state` карта, события `startup`/`reconnect` с `downSec`/`errors`, `cause` из NVS (`device-log`), null-проверка `state` в `callbackMQTT()`.
- Известный баг: перегрузка `publish(const char*, const char*, boolean)` публикует данные как retained — нужен явный `publish(topic, (const uint8_t*)buf, len, false)`; `subscribe()` не проверяется.

### Реестр устройств (`saspik-iot/devices.md`)

| objectId | Прошивка | Тип |
| --- | --- | --- |
| `saspik-gwqm001` | esp32-controller | шлюз ESP-NOW→MQTT + реле |
| `saspik.sa.wm.m003` | esp32-local-mqtt | сенсор DHT22+LED (legacy, на своей прошивке) |
| `saspik.s.en.b001` | saspik-iot-node-sensor-1 | сенсор ESP-NOW battery |
| `saspik.s.en.b002` | esp32-battery-check | сенсор ESP-NOW battery |
| `saspik.s.en.b000` | esp32-node-template | шаблон |

- Фактические id текущей автоматики: `saspik-awqm001` (`unitId3`, local-mqtt), `saspik-aenm001` (`unitId2`, boilerplate device). В реестре они не зафиксированы, а строка `esp32-local-mqtt` всё ещё указывает `m003` — реестр отстаёт.
- Классификатор требует `saspik-{роли}{протокол}{питание}{nnn}`; `saspik.sa.wm.m003` — старый формат (точки, `wm`), переименование не выполнялось по решению.

## InfluxDB: бакеты и запросы

- Бакеты: `INFLUXDB_BUCKET` (`mqtt`, без retention) и `INFLUXDB_LOGS_BUCKET` (`logs`, retention `INFLUXDB_LOGS_RETENTION_DAYS`=7).
- Провижининг: `influxdb/provision/bootstrap.sh` → `ensure-buckets.sh`; нефатален; entrypoint-обёртка `/provision/bootstrap.sh` в compose; `DOCKER_INFLUXDB_INIT_*` — первичная настройка.
- Серверный Flux (`influxDbStateStore.service.ts`): `from(bucket) |> range(-30d) |> filter(topic == X or topic == X/state) |> filter(_field == key|state) |> last() |> group() |> sort(_time desc) |> limit(1)`; строки экранируются.
- `group()` и фиксированный `[agent] hostname` критичны (инцидент «статики» 2026-09-20): без них `last()` возвращает по таблице на серию, и UI показывает мёртвую серию.
- Шаблон запроса логов: `influxdb/queries/logs-view.flux`; пример — в [`docs/logs.md`](saspik-cluster/docs/logs.md).

## Конфигурация docker-compose

| Сервис | Образ/сборка | Порты | Лимиты CPU/RAM | Healthcheck |
| --- | --- | --- | --- | --- |
| backend | `server/Dockerfile` (node:18-alpine) | 3001 | 0.5 / 512M | `wget http://127.0.0.1:3001/health`, 60 с |
| nginx | `nginx/Dockerfile` | 80, 443 | 0.3 / 256M | `wget --spider /health`, 60 с |
| mongo | `mongo:7` | — | 0.5 / 512M | `mongosh db.adminCommand('ping')`, 60 с |
| mosquitto | `mqtt/Dockerfile` | 1883, 9001 | 0.2 / 128M | `mosquitto_pub healthcheck/ping`, 30 с |
| mqtt-rule-engine | `mqtt-rule-engine/Dockerfile` | — | 0.3 / 128M | — |
| influxdb | `influxdb/Dockerfile` (2.9.1) | 8086 | 0.3 / 256M | — |
| telegraf | `telegraf/Dockerfile` | — | 0.2 / 128M | — |

- Все сервисы: `restart: unless-stopped`, сеть `fullstack-network` (bridge), reservations ≈ половина лимитов.
- Volumes: `mongo-data`, `influxdb-data`. ACL и конфиги mosquitto вшиты в образ; nginx кэширует статику на год, проксирует `/api` → backend.
- Переменные окружения подставляются из `.env` (`docker compose`), `NODE_ENV` для backend — `prod`.

## Хроника решений

Журналы `IN_PROGRESS.md` сейчас ведутся без дат; ниже — ключевые вехи (даты из исторических записей).

- **2026-09-03** — единый JSON-конверт логов + фиксы healthcheck (mosquitto, server wget, nginx 127.0.0.1). Коммиты `268dd72`, `7cebe4c`.
- **2026-09-04** — стабилизация `esp32-local-mqtt`: partitions 4MB «spiffs», upload 115200, неблокирующий MQTT-connect, отказ от файлового лога (только NVS); мок DHT22.
- **2026-09-05** — инцидент `healthcheck/ping`: не-JSON payload ронял батч telegraf → payload стал JSON, топик исключён из входов.
- **2026-09-20** — «статика» значений: фикс `[agent] hostname="telegraf"` + `group()` в Flux-запросе (коммиты `8948847`, `c57169a`, `9a4f872`).
- **2026-09-28…10-14** — анализ логов `unitId2`, план выравнивания, мёртвые ветки логов (guard `!mqttClient.connected()`), пост-мортем логирования (`reconnect`/`startup` с `downSec`/`errors`, `prevUptime`).
- **2026-10-19/20** — boilerplate + host-тесты; план UI-фиксов (кнопки «назад», роут объекта, wildcard-статусы); сохранение topic при `updateObject` (инвариант 2).
- **ACL `device/#`** (`a9f4b43`) — команды и retained-состояния перестали отбрасываться; проверено end-to-end на проде (`185.72.145.19`). Урок: ACL в образе, нужен rebuild.
- **Конфигурируемые каналы** (`a314468`) — команда `device/{unitId}/{objectId}/command` с `{"channel","value"}`, `CHANNELS[]` в прошивке, telegraf `json_string_fields=["*"]`, экранирование Flux; объекты `saspik-awqm001` (`unitId3`) и `saspik-aenm001` (`unitId2`).
- **Правила автоматизации `unitId3`** (`4bdd3ea`) — `m003_led_*` в сиде + аддитивный seed правил (добавляет только отсутствующие `id`).

## Rightech

- Константа `MQTT_BROCKER_API_URL = "https://dev.rightech.io/api/v1"` в `server/src/const.ts` (опечатка «BROCKER» — историческая).
- `TYPES.RightechProxyMqttService` биндится на `LocalMqttService`; `MqttService` — декоратор с логом «initialized with Rightech proxy», фактически делегирует локальному клиенту mosquitto.
- `RIGHTECH_API_TOKEN` присутствует в `example.env` и в compose-окружении, но код его не читает; сам токен просрочен (подтверждено владельцем) и секретом не является.
- DTO `rightechObject.dto.ts`, `rightechModel.dto.ts` остались от ранней интеграции; в роутах не используются.
- Вывод: интеграции нет; вопрос «выпиливать или развивать» — открытый.

## Известные проблемы и TODO

- `esp32-local-mqtt`: retained-публикация из-за перегрузки `publish`; непроверяемый `subscribe()`; мок DHT22 (физ. датчик не подключён) — статус в `saspik-iot/IN_PROGRESS.md`.
- `server/src/services/objects/objectsService.service.ts`: `getLastSensorsData` возвращает `{}`.
- `validate.middleware.ts` не подключён — валидации DTO нет; `ExeptionFilter` (sic) зарегистрирован.
- `ClimateControlService` отключён (`executeControll` закомментирован в `app.ts`), `RelayControllerOptions` биндится пустым `{}`; климат фактически считает rule-engine.
- Backend без тестов: `npm test` — заглушка. `npm run lint` может падать на `Object.hasOwn` (старый Node/ESLint), к правкам не относится.
- `saspik-iot/devices.md` без `saspik-awqm001`/`saspik-aenm001` и с устаревшей строкой `m003` (решение владельца: пока не править).
- Креды по умолчанию (`admin/password123`, `admin123`) в compose, `telegraf.conf`, `example.env` — нельзя в прод без смены.
- `MqttController` не смонтирован; `GET /history/list` не существует; HistoryService — план.
- Правила климат-контроля дублируются на unitId; `unitId3`-правила ссылаются на legacy `m003` (новые устройства их не используют).
- `unitId3`/`saspik-awqm001` нет в сиде — объект существует только в Mongo (создан через UI).
- Генератор `docs.html` — приватный скилл `docs-gen` вне репозитория; воспроизводимость генерации не гарантирована.
- Прод-деплой (`185.72.145.19`) и миграции Mongo выполняются вручную; процедура не зафиксирована.
