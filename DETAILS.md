# DETAILS.md

Расширение к [`PROJECT_CONTEXT.md`](PROJECT_CONTEXT.md). Содержит детали, не вошедшие в основной документ из-за бюджета токенов. Ссылки из основного документа вида → DETAILS#раздел.

## 1. Стек: полные версии

### server (`saspik-cluster/saspik-cluster-app/server/package.json`)

- deps: express ^4.19.2, inversify ^6.0.2, mongodb ^6.10.0, mqtt ^5.0.0, @influxdata/influxdb-client ^1.35.0, axios ^1.4.0, body-parser ^1.20.2, class-transformer ^0.5.1, class-validator ^0.14.1, cors ^2.8.5, dotenv ^17.2.3, reflect-metadata ^0.2.2, tslog ^4.9.3.
- devDeps: typescript ^5.5.4, ts-node ^10.9.2, nodemon ^3.1.4, eslint ^9.7.0, prettier ^3.3.3, @typescript-eslint/* ^8.26.0, eslint-plugin-import, eslint-config-prettier, eslint-plugin-prettier.
- Node-версия образа: `node:18-alpine` (server/Dockerfile).

### client (`client/package.json`)

- react ^19.1.0, react-dom ^19.1.0, react-router-dom ^6.30.3, react-scripts 5.0.1, typescript ^4.9.5, bem-react-classname ^1.3.2, @ant-design/icons ^6.1.0, web-vitals ^2.1.4.
- Тесты: @testing-library/react ^16.3.0, jest-dom, user-event, @testing-library/dom.
- engines: node >= 18.

### mqtt-rule-engine (`mqtt-rule-engine/package.json`)

- mqtt 5.3.5, mongodb ^6.10.0; dev: typescript ^5.7.3, ts-node, eslint ^9.17.0, @types/node ^22.10.6.
- engines: node >= 18.

### Прошивки

- `esp32-local-mqtt/platformio.ini`: platform espressif32, framework arduino; libs: PubSubClient ^2.8, ArduinoJson ^6.21.3, DHT ^1.4.4, OneWire ^2.3.8; shared libs (wifi-config, device-log); custom `partitions.csv` (4MB, раздел `spiffs`), `upload_speed=115200`, `board_build.filesystem=littlefs`.
- `esp32-boilerplate/platformio.ini`: env `{gateway,sensor,device}_{esp32,s2,s3,c3}` + `native` (host-тесты, Unity, `lib_ldf_mode = deep+`, `-DDEVICE_ROLE`).
- Строительные флаги ролей: `-DDEVICE_ROLE=1|2|3` (Gateway/Sensor/Device).

## 2. HTTP API: точные роуты и хендлеры

### Монтирование (`server/src/const.ts`, `server/src/app.ts`)

- Префикс `/api/v1`; домены: `/objects`, `/units`, `/rules`, `/mqtt` (последний НЕ смонтирован).
- Вне префикса: `GET /health`, `GET /api/getTimestamp`.

### UnitsController (`server/src/controllers/units.controller.ts`)

- `GET /units/list` → список юнитов с objects+rules.
- `GET /units/:id` → юнит; 404 если нет.
- `POST /units` → создание `{id, name, description?}`.
- `PATCH /units/:id` → `{name, description?}` (id не меняется).
- `DELETE /units/:id` → каскадно удаляет объекты юнита (`UnitsService.deleteUnit`: `objectsRepository.deleteByUnitId` + удаление юнита; правила не трогаются).

### ObjectsController

- `POST /objects/list/:type` (`sensor|device`) + body `{unitId?}` → объекты, обогащённые значениями каналов (`enrichObject`: `{key, value, spec}`, `updatedAt` из max-таймстампа, `status` из `deviceStatuses`).
- `POST /objects/getByIds` body `{id[], type?, unitId?}`.
- `POST /objects/command/:deviceId` body `{value, unitId?}` → publish в топик объекта `{specKey: value}`; specKey берётся из `spec[0].key` или `"state"` (`objectsService.service.ts:82`).
- `POST /objects/getLastSensorsData` — заглушка, возвращает `{}`.
- `POST /objects` → создание: схлопывает постфикс, дописывает по типу, строит topic `${type}/${unitId}/${base}`.
- `PATCH /objects/:id` → обновление с сохранением неканонического topic.
- `DELETE /objects/:id` body `{unitId}`.

### RulesController

- `GET /rules?unitId=...` → правила в формате rule-engine (движок сам фильтрует `enabled:false` на своей стороне).
- `POST /rules` → upsert по `id`.
- `PATCH /rules/:id` → частичное обновление (`{enabled: true/false}` — мягкое отключение).
- `DELETE /rules/:id`.

## 3. MQTT: детальные контракты

### Домены (docs/mqtt-topics.md)

| Домен | Направление | Пример |
| --- | --- | --- |
| `sensor/{unitId}/{objectId}` | device → broker | `sensor/unitId2/saspik.sa.wm.m002` |
| `device/{unitId}/{objectId}` | broker → device (команды) | `device/unitId2/saspik.sa.wm.m002` |
| `device/{unitId}/{objectId}/state` | device → broker (retained) | `{"state":"ON"}` |
| `device/{unitId}/{objectId}/status` | device → broker (retained) + LWT | `{"status":"online"}` / LWT `{"status":"offline"}` |
| `device/{unitId}/{objectId}/log` | device → broker (логи) | конверт лога |
| `units/{unitId}/commands/a_relay1..4` | rule-engine/server → broker | `{"state":"1"}` |
| `units/{unitId}/sensors` | агрегированные данные | — |
| `server/worker/log`, `rule-engine/worker/log` | логи | конверт лога |
| `healthcheck/ping` | healthcheck mosquitto | `{"payload":"test"}` |

### Команды объектов (device) vs команды климата (units)

- `device/...` — payload `{"state":"ON"|"OFF"}` (строки ON/OFF), парсится прошивкой `callbackMQTT` (esp32-local-mqtt).
- `units/{unitId}/commands/a_relay*` — payload `{"state":"1"|"0"}` (числа как строки), см. `rules.json` `_meta.commands`.
- ВАЖНО: форматы не взаимозаменяемы; `ObjectsService.callCommand` шлёт `{specKey: value}` в `device/{unitId}/{objectId}`.

### ESLint-контракт ACL (mqtt/mosquitto.acl)

- `healthcheck/#`, `clients/%c/#` (namespace по Client ID), `sensor/#`, `units/#`, `device/+/+/log`, `server/+/log`, `rule-engine/+/log`.
- Default-deny: всё остальное заблокировано.

### Telegraf-входы (telegraf.conf)

- Данные: `sensor/#`, `units/#`, `led/#`, `device/+/+/state` → measurement `mqtt_consumer`, `json_string_fields=["state"]`.
- Логи: `device/+/+/log`, `server/+/log`, `rule-engine/+/log` → measurement `logs` (`name_override="logs"`), теги `[topic, level, event, src, unitId, objectId]`, `json_string_fields=["msg","cause"]`.
- Outputs: два `influxdb_v2` (bucket `INFLUXDB_BUCKET` c namepass `mqtt_consumer`; bucket `INFLUXDB_LOGS_BUCKET` c namepass `logs`), `flush_interval="1s"`, `hostname` фиксирован.

## 4. Правила климат-контроля (server/data/rules.json)

- Формат: `_meta` (описание, data_sources, commands, temperature_zones, @todo) + `rules[]`.
- Правила: `temp_emergency_high`, `temp_emergency_low`, `fan_on`, `fan_off`, `humidifier_on`, `humidifier_off`, water-правила (и др.) — с env-подстановкой:
  - `${CLIMATE_CONTROL_UNIT_ID}`, `${CLIMATE_T_SET}`, `${CLIMATE_T_HYST}`, `${CLIMATE_T_MAX}`, `${CLIMATE_T_MIN}`, `${CLIMATE_T_RECOVERY}`, `${CLIMATE_RH_SET}`, `${CLIMATE_RH_HYST}` и ночные `CLIMATE_NIGHT_*`; `${NIGHT_START_HOUR}`, `${NIGHT_END_HOUR}`.
  - Выражения: `${expr:CLIMATE_T_SET + CLIMATE_T_HYST / 2}` — safe-eval без eval() (парсер в `seed.service.ts` и `mqtt-rule-engine/src/config/loader.ts`).
- Условия в правилах: `jsonpath`, `timeBetween` (поле `$.timestamp`), композиты `and/or/not`.
- Действия: `publish` (в `units/.../commands/a_relay*`) и `log`.
- Ограничение: `_meta/@todo` — правила продублированы на каждый unitId, т.к. движок не умеет подставлять unitId из wildcard-топика.
- Сид применяет `enabled:true` ко всем правилам; `unitId` вычисляется из топика (`resolveRuleUnitId`) → в `rules.json` unitId отсутствует, он докидывается при сиде.

## 5. Rule Engine: устройство

- `src/index.ts` — вход: `MqttAdapter` + `RuleEngine` + `createRulesProvider()` (mongo|api|file по `RULES_SOURCE`).
- Ядро `RuleEngine` (`src/core/engine.ts`): индекс `Map<topic, Rule[]>`, exact + wildcard-матчинг (`topicMatches`), переподписка при `loadRules`, не логирует каждое входящее сообщение.
- Контекст `MessageContext.fromMqttPacket` — JSON-парсинг payload, JSONPath-доступ.
- Условия (`src/conditions/`): `topicRegex`, `jsonpath`, `payloadEquals`, `timeBetween`, композиты `and/or/not`; регистрация через `src/conditions/constants.ts` + `src/config/builder.ts`.
- Действия (`src/actions/`): `publish` (плейсхолдеры `{{field}}`), `log` (в `rule-engine/worker/log`), `timeout` (отложенное выполнение).
- Логи: `rule-fired` / `rule-error` события через `src/logging`.
- Тесты: `npm run test` → `test/test-runner.ts` с `mock-mqtt-adapter.ts` и `sample-rules.json`.

## 6. Прошивки: протоколы и подтверждённые уроки

### ESP-NOW v2 (esp32-boilerplate)

- Кадр: `[type(1)] [timestamp LE(4)] [fieldId(1)+len(1)+data]* [crc8(1)]`; типы 1=DATA, 2=LOG, 3=COMMAND, 4=ACK.
- Служебные fieldId: `0xFF` — JSON лога, `0xFE` — JSON команды; CRC-8/MAXIM (полином 0x8C, init 0x00); лимит пакета 250 байт.
- Несовместим с v1 (saspik-iot-node-sensor-1): иной формат/CRC (см. `esp32-controller` — CRC8 из библиотеки).
- Модули saspik-core: config (NVS, namespace `saspik_app`, merge defaults←NVS), portal (captive portal + `wifi-config`), mqtt, espnow, bridge, log (UART+MQTT/ESP-NOW), sys (LightSleep), interfaces (bus I2C/OneWire/SPI/UART, sensor/actuator).

### esp32-local-mqtt (действующий сенсор)

- В `config.h` сейчас: `OBJECT_ID="saspik.sa.wm.m003"`, `UNIT_ID="unitId3"`, `OBJECT_TYPE="sensor"`, `USE_MOCK_SENSOR=true` (физ. DHT не подключён), PIN_LED=2 active-low, CONFIG_BUTTON_PIN=25, таймауты ребута MQTT 15 мин / WiFi 5 мин, `MQTT_RECONNECT_INTERVAL_MS=3000`, `MQTT_CONNECT_TIMEOUT_MS=4000`, `SENSOR_INTERVAL_MS=2000`.
- Расхождение: README/IN_PROGRESS фиксируют m002/unitId2 → TODO: проверить эталон.
- Авто-восстановление WiFi в loop, LWT offline, retained online, retained state LED, лог-события startup/reconnect с `downSec`/`errors`, `cause` из NVS (`device-log`).

### Реестр устройств (saspik-iot/devices.md)

| objectId | Прошивка | Тип |
| --- | --- | --- |
| `saspik-gwqm001` | esp32-controller | шлюз ESP-NOW→MQTT + реле |
| `saspik.sa.wm.m003`* | esp32-local-mqtt | сенсор DHT22+LED |
| `saspik.s.en.b001` | saspik-iot-node-sensor-1 | сенсор ESP-NOW battery |
| `saspik.s.en.b002` | esp32-battery-check | сенсор ESP-NOW battery |
| `saspik.s.en.b000` | esp32-node-template | шаблон |

\* — старый формат (точки, `wm`), классификатор требует `saspik-sawqm...`; переименование не выполнялось по решению.

## 7. InfluxDB: бакеты и характерные запросы

- Бакеты: `INFLUXDB_BUCKET` (`mqtt`, без retention) и `INFLUXDB_LOGS_BUCKET` (`logs`, retention `INFLUXDB_LOGS_RETENTION_DAYS`=7).
- Провижининг: `influxdb/provision/bootstrap.sh` → `ensure-buckets.sh`; нефатален; entrypoint-обёртка `/provision/bootstrap.sh` в compose.
- Серверный Flux (`influxDbStateStore.service.ts:40`): `from(bucket) |> range(-30d) |> filter(topic == X or topic == X/state) |> filter(_field == key|state) |> last() |> group() |> sort(_time desc) |> limit(1)`.
- `group()` и фикс. hostname — критичны (инцидент «статики» 2026-09-20).
- Шаблон запроса логов: `influxdb/queries/logs-view.flux`.

## 8. Конфигурация docker-compose: сводка сервисов

| Сервис | Образ/сборка | Порты | Лимиты CPU/RAM | Healthcheck |
| --- | --- | --- | --- | --- |
| backend | server/Dockerfile | 3001 | 0.5 / 512M | wget /health |
| nginx | nginx/Dockerfile | 80, 443 | 0.3 / 256M | wget /health |
| mongo | mongo:7 | — | 0.5 / 512M | mongosh ping |
| mosquitto | mqtt/Dockerfile | 1883, 9001 | 0.2 / 128M | mosquitto_pub healthcheck/ping |
| mqtt-rule-engine | mqtt-rule-engine/Dockerfile | — | 0.3 / 128M | — |
| influxdb | influxdb/Dockerfile (2.9.1) | 8086 | 0.3 / 256M | — |
| telegraf | telegraf/Dockerfile | — | 0.2 / 128M | — |

- Сеть: `fullstack-network` (bridge); volumes: `mongo-data`, `influxdb-data`.
- `restart: unless-stopped` у всех сервисов.

## 9. Хроника ключевых решений (IN_PROGRESS.md)

- 2026-09-03: единый JSON-конверт логов + фиксы healthcheck (mosquitto, server wget, nginx 127.0.0.1).
- 2026-09-04: стабилизация esp32-local-mqtt: partitions 4MB «spiffs», upload 115200, неблокирующий MQTT-connect, отказ от LittleFS-лога (только NVS); мок DHT22.
- 2026-09-05: инцидент `healthcheck/ping` (не-JSON ронял батч telegraf) — payload переведён в JSON, топик исключён из telegraf.
- 2026-09-20: «статика» значений: фикс `hostname="telegraf"` + `group()` в Flux-запросе сервера.
- 2026-09-28/10-14: анализ логов unitId2, план выравнивания m002; выявлены мёртвые ветки логов (guard `!mqttClient.connected()`), принят план эталона m002.
- 2026-10-14: пост-mortem логирования esp32-local-mqtt — события `reconnect`/`startup` с `downSec`/`errors`, `prevUptime` в NVS, защита от `strcmp(nullptr)` в callbackMQTT.
- 2026-10-19: закрытие пунктов бэклога boilerplate'ом; коммит esp32-boilerplate + host-тесты.
- 2026-10-20 (cluster): план UI-фиксов (кнопки назад, битый роут `/object/:objectId`, wildcard-подписка `device/+/+/status`), Этап 3 — сохранение topic при updateObject (в коде уже есть `isCanonicalTopic`), эталон unitId2, миграция Mongo (зависит от docker).
- 2026-10-20 (iot): план esp32-relay-x8 (реле на GPIO32,33,25,26,27,14,12,13; командные топики `device/unitId3/saspik-awqm001/relay1..8`, retained state, восстановление из retained после ребута) — реализация не начата.

## 10. Известные проблемы и TODO (из кода и документации)

- `server/src/services/objects/objectsService.service.ts`: `getLastSensorsData` возвращает `{}`.
- `server/src/errors/` — `ExeptionFilter` зарегистрирован, но `validate.middleware.ts` в контроллерах не используется (нет валидации входных DTO).
- `server/src/services/climate-control/climateControl.service.ts`: `executeControll()` закомментирован в `app.ts` — сервис жив, цикл не запускается; `RelayControllerOptions` биндится пустым `{}`.
- `server/package.json` `test` — заглушка.
- TODO.md кластера: улучшение типизации (`TEMPORARY_ANY`), репозиторный слой (частично внедрён), OpenAPI-документация, кэширование, защита от DDoS, выключение автоматизации (rule engine) через UI/API — не реализовано.
- `MqttController` не смонтирован — publish/subscribe API недоступны.
- Rightech: `MQTT_BROCKER_API_URL` и `RightechProxyMqttService` — без фактической реализации.
- docs/HistoryService.md — раздел-заглушка «запланировано».
- Файл `_temp.md` в корне — вероятно временный, проверить перед удалением.