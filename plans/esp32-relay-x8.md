# ⧫ План — прошивка esp32-relay-x8

#### Восемь реле на заводской ESP32 Relay Board x8: управление из UI и правил, retained-состояние

Статус: план согласован, реализация не начата.
Связанная задача — [`saspik-iot/IN_PROGRESS.md`](../saspik-iot/IN_PROGRESS.md) → «Прошивка esp32-relay-x8».

## Бизнес-результат

Владелец получает готовое исполнительное устройство на 8 реле вместо самодельной платы: реле переключаются из UI и из правил кластера, состояние видно в карточке объекта и переживает перезапуск. Ошибка монтажа или прошивки ловится на стенде, а не на объекте.

## Что переиспользуем

- `esp32-boilerplate` — база скетча (composition root, `config.h`-суперсет, `partitions.csv`).
- `saspik-core` — `MqttManager` (LWT, retained `{"status":...}`, авто-ребут, переподписка), `Logger`/`MqttLogSink` (лог-конверт), `ConfigManager`/`NvsConfigStore`, `CaptivePortal`, `TopicBuilder`.
- `esp32-boilerplate` — `ChannelBank` (сборка каналов из `CHANNELS[]`, `applyCommand`, `writeState`, `refreshActuators`) и `GatewayApp` как composition root.
- Новое в `saspik-core` — `StateRestorer`: подписка на собственный `/state`, grace-таймер, применение retained-карты без эха. Пишется один раз и переиспользуется другими ролями.
- **Новых классов «под реле» не создаём**: 8 реле — это 8 записей `CHANNELS[]` с `role = Actuator`. Отдельные `RelayBank`/`RelayCommandHandler`/`RelayApp` не нужны.

## Решения

1. **objectId — `saspik-awqm002`, `unitId3`.** Шифр `a`+`wq`+`m` (actuator, WiFi+MQTT, mains). `saspik-awqm001` уже занят `esp32-local-mqtt`; два устройства на один objectId дают конфликт топиков и одну карточку в UI. Требует подтверждения владельца по `AGENTS.md`.
2. **Команды — один `/command` на объект.** `device/unitId3/saspik-awqm002/command`, payload `{"channel":"relay1","value":"ON"|"OFF"}`; состояние — retained JSON-карта всех каналов. Старый вариант «топик на каждое реле» (`…/relay1` с `{"state":"ON"}`) отклонён: нарушает контракт `AGENTS.md` и не проходит через `spec`-проверку сервера.
3. **Дорабатываем `GatewayApp`, новый шаблон `RelayApp` не заводим.** Восстановление выносим в переиспользуемый компонент `StateRestorer` в `saspik-core`: подписка на свой `/state`, grace ~3 с после подключения, применение карты через `ChannelBank` без эха. `GatewayApp` подключает его по флагу `RESTORE_STATE_ON_CONNECT` из `config.h` (по умолчанию `false` — поведение существующих устройств не меняется). Почему не `RelayApp`: он на ~80% повторяет `GatewayApp` (портал, логгер, `ConfigManager`, таймеры, статус-LED) — это дублирование, запрещённое `firmware-gen`. Нулевой вариант (без восстановления: после ребута все реле OFF, fail-safe) остаётся допустимым, если владельцу не нужно сохранять включённое состояние.
4. **Дубль в `esp32-dht22-led` убираем.** Копии `GatewayApp.*`, `IApp.h`, `ChannelBank.*` в его `src/app` побайтово совпадают с boilerplate; иначе правка в boilerplate до устройства не доедет. Прошивка должна собираться против boilerplate (`lib_extra_dirs`), без собственных копий шаблонов.

## Контракт

| Топик | Направление | Payload |
| :--- | :--- | :--- |
| `device/unitId3/saspik-awqm002/command` | broker → device | `{"channel":"relay1","value":"ON"\|"OFF"}` |
| `device/unitId3/saspik-awqm002/state` | device → broker, retained | `{"relay1":"ON",…,"relay8":"OFF"}` |
| `device/unitId3/saspik-awqm002/status` | device → broker, retained + LWT | `{"status":"online"}` / `{"status":"offline"}` |
| `device/unitId3/saspik-awqm002/log` | device → broker | лог-конверт `{level,src,event,msg,topic,unitId,objectId,uptime,cause}` |

- ACL: домен `device/#` уже разрешён — правка `mosquitto.acl` не нужна.
- Telegraf: `device/+/+/state` и `device/+/+/log` уже в подписках — правка не нужна.
- UI: в `spec` объекта — 8 ключей `relay1..relay8` (тип переключатель), иначе сервер не пропустит команду.

## Железо

| Канал | GPIO | Примечание |
| :--- | :--- | :--- |
| `relay1` | 32 | |
| `relay2` | 33 | |
| `relay3` | 25 | |
| `relay4` | 26 | |
| `relay5` | 27 | |
| `relay6` | 14 | |
| `relay7` | 12 | strapping (MTDI): не тянуть вверх на старте |
| `relay8` | 13 | |

- `HIGH` = реле включено; на старте все пины `LOW` — инициализация первым шагом `setup()` («none go HIGH at startup»).
- Питание реле — 5 В от платы, логика 3.3 В; общий GND обязателен (записать в README).
- Кнопка портала — GPIO0/BOOT (штатное поведение boilerplate).

## Состав скетча

```
saspik-iot-sketches/esp32-relay-x8/
├── platformio.ini        # env relay_x8_esp32, lib_extra_dirs: ../shared, ../esp32-boilerplate/lib,
│                         # lib_ldf_mode = deep+, -Iinclude, partitions.csv
├── partitions.csv        # 4MB, раздел "spiffs" (из boilerplate)
├── include/config.h      # UNIT_ID, OBJECT_ID, CHANNELS[] (8 actuators), RESTORE_STATE_ON_CONNECT, таймауты
├── src/main.cpp          # создаёт GatewayApp (boilerplate), без своей логики WiFi/MQTT
└── README.md             # по шаблону ⧫: параметры, сборка, схема подключения, strapping GPIO12
```

Изменения вне скетча:

- `saspik-core` — новый компонент `StateRestorer` (+ host-тест на разбор карты и grace-логику).
- `GatewayApp` boilerplate — проводка `StateRestorer` под флагом (~15–20 строк), без изменения поведения при `false`.
- `esp32-dht22-led` — удалить копии `src/app/GatewayApp.*`, `IApp.h`, `ChannelBank.*`, собирать против boilerplate.
- Регистрация: строка в `saspik-iot/devices.md`, пункт в `saspik-iot-sketches/README.md`, топики — в `saspik-cluster/docs/mqtt-topics.md`, регенерация `docs.html`.

## Чек-лист реализации

1. `config.h`: `OBJECT_ID`/`UNIT_ID`, 8 каналов `relay1..8` (роль Actuator, пины, `activeLow = false`), `CLIENT_ID_PREFIX = "saspik-a"`, `RESTORE_STATE_ON_CONNECT = true`.
2. `saspik-core`: `StateRestorer` — подписка на `device/{unitId}/{objectId}/state`, разбор JSON-карты, применение через `ChannelBank` без публикации, grace-таймер и «baseline all-OFF», если retained не пришёл.
3. `GatewayApp`: подписка на свой `/state` и вызов `StateRestorer` только при `RESTORE_STATE_ON_CONNECT`; при `false` — поведение байт-в-байт прежнее.
4. `esp32-dht22-led`: удалить копии `src/app/GatewayApp.*`, `IApp.h`, `ChannelBank.*`; сборка — против boilerplate через `lib_extra_dirs`.
5. `src/main.cpp` скетча: только создание `GatewayApp` и `begin()`/`update()` — без своей логики WiFi/MQTT.
6. `README.md` скетча: пины, активный уровень, strapping GPIO12, сборка/прошивка, ручные сценарии.
7. `devices.md` и README скетчей — строки по шаблону, без дублей objectId.

## Приёмка

- `pio run -e relay_x8_esp32` — сборка успешна.
- `pio test -e native` в boilerplate — host-тесты не сломаны, включая тест `StateRestorer`.
- `pio run -e gateway_esp32` (boilerplate) и `pio run` в `esp32-dht22-led` после удаления копий — обе сборки успешны, поведение устройства без флага не изменилось.
- На стенде: все 8 реле переключаются `mosquitto_pub` в `/command`; `state` — retained и содержит все 8 ключей; после перезагрузки платы состояние восстанавливается при `RESTORE_STATE_ON_CONNECT = true`, а при отсутствии retained публикуется baseline «все OFF»; устройство видно в UI как `online`, карточка отражает состояние; логи приходят в бакет `logs`.

## Риски

- **Конфликт objectId с `esp32-local-mqtt`** — решается решением 1, до кода.
- **GPIO12 strapping**: подтяжка вверх на старте мешает загрузке — реле 7 не должно «дёргаться» при включении.
- **Правка общего `GatewayApp` затрагивает боевой `esp32-dht22-led`** — закрывается флагом по умолчанию `false` и обязательными сборками `gateway_esp32` + `esp32-dht22-led` после правки.
- **Расхождение копий шаблона** — пока в `esp32-dht22-led` лежат копии `GatewayApp`, любая правка boilerplate до устройства не доходит; копии удаляются в этой же работе.
- **Лимит payload 128 байт** (`MQTT_PAYLOAD_MAX`): retained-карта на 8 каналов занимает ~113–121 байт — проходит, но запаса почти нет. Добавление 9-го канала или длинных имён потребует увеличить лимит в `config.h` и перепроверить буферы.
- Вне скоупа: регистрация объекта в Mongo/сиде, production-ACL, интеграция climate-control с `unitId3`, ack/heartbeat (отдельная задача «Гарантия исполнения команд»).

## Промпт для `firmware-gen`

```text
Создай прошивку esp32-relay-x8 по плану plans/esp32-relay-x8.md.

Параметры: роль actuator, протокол WiFi+MQTT, питание mains.
objectId saspik-awqm002, unitId3, env relay_x8_esp32.
Каналы: relay1..relay8 на GPIO 32, 33, 25, 26, 27, 14, 12, 13; HIGH = ON,
activeLow = false, на старте все LOW.

База — только esp32-boilerplate: переиспользуй GatewayApp, ChannelBank,
MqttManager, Logger, ConfigManager, CaptivePortal. Новых RelayApp/
RelayBank/RelayCommandHandler не создавай — 8 каналов это записи CHANNELS[].
Команды — один device/unitId3/saspik-awqm002/command с {"channel","value"};
state — retained карта всех каналов; логи и status — как в boilerplate.

Восстановление состояния из retained реализуй компонентом StateRestorer в
saspik-core (подписка на свой /state, grace ~3 с, применение через
ChannelBank без эха, baseline «все OFF» если retained не пришёл).
GatewayApp подключает его по флагу RESTORE_STATE_ON_CONNECT из config.h,
по умолчанию false — поведение существующих устройств не меняется.

Убери дубль: в esp32-dht22-led лежат копии src/app/GatewayApp.*, IApp.h,
ChannelBank.*, идентичные boilerplate. Удали их и переведи прошивку на
сборку против boilerplate через lib_extra_dirs.

Собери pio run -e relay_x8_esp32, pio run -e gateway_esp32 в boilerplate,
pio run в esp32-dht22-led и pio test -e native. Добавь host-тест на
StateRestorer, README по шаблону ⧫, строку в devices.md и в README
скетчей. Топики — в saspik-cluster/docs/mqtt-topics.md.
Прошивку на железо и ручные сценарии оставь владельцу.
```
