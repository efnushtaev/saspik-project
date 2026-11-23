---
name: firmware-gen
description: Создаёт новый скетч IoT-устройства в saspik-iot-sketches — только на базе esp32-boilerplate, с приоритетом готовых классов saspik-core, objectId по классификатору, README по шаблону ⧫, регистрация в devices.md и сборка pio run.
whenToUse: Запрос «создай прошивку», «создай скетч», «добавь iot-устройство».
---

# Firmware Generator

Новый скетч в `saspik-iot/saspik-iot-sketches/<название>/`.

## 0. Принципы

- **База — только `esp32-boilerplate`.** Не копируй `esp32-local-mqtt` или другой
  скетч как основу новой прошивки.
- **Сначала `saspik-core` и шаблоны boilerplate.** Максимально используй
  готовые классы и реализации ядра; не дублируй их в скетче.
- **Потом `esp32-boilerplate/src/app/examples`.** Если реализации нет в core
  (например, `Dht22Sensor`, `FloatSensor`, `RelayActuator`), бери её оттуда.
  Если класс нужен нескольким прошивкам — вынеси его в `saspik-core` отдельным
  решением, не плоди копии.
- **Новый код — последним.** Только если готового класса действительно нет.

## 1. Параметры устройства

Уточни, если не указано в запросе:

- **Роль**: `g` (gateway) / `s` (sensor) / `a` (actuator), можно комбинировать.
- **Протокол**: `wq` (WiFi+MQTT) / `en` (ESP-NOW) / `lr` (LoRa).
- **Питание**: `m` (mains) / `b` (battery).
- **Модули**: какие датчики, исполнители и шины будут реально использоваться
  (DHT22, LED, реле, геркон, I2C/SPI/UART, …). Если пользователь сам не
  перечислил — спроси; без этого нельзя подобрать готовый драйвер.
- **Подбор драйвера**: проверь `saspik-core/drivers`; если готового нет —
  `src/app/examples`; если модуль переиспользуемый — добавь драйвер в
  `saspik-core`, а не копируй в скетч.
- **Название папки** и краткое описание для таблицы и README.
- **Железо**: пины, периферия — для `config.h` и README.

## 2. objectId и unitId

- Формат `saspik-{роли}{протокол}{питание}{nnn}` по [`docs/classification.md`](../../../saspik-iot/docs/classification.md). Источник кодов — секция «Формат»; заголовок файла с точками устарел, действует дефисный формат.
- `nnn`: `000` — шаблон, `001+` — серийные.
- Занятость проверь по `saspik-iot/devices.md` и `grep` по `OBJECT_ID` в `saspik-iot-sketches/**/config.h`.
- `unitId` — свободный (`unitId1`, `unitId2`, …); занятые — в `devices.md`, плюс учти планы из `saspik-iot/IN_PROGRESS.md` (заявленные, но ещё не созданные устройства).

## 3. База

Новая прошивка создаётся **только из `esp32-boilerplate`** — не копированием
`esp32-local-mqtt` или другого скетча. Сначала максимально переиспользуй
готовые классы:

- **`saspik-core`**: `ISensor` / `IActuator`, `SensorSample`, `MqttManager`,
  `Logger` / `MqttLogSink`, `ConfigManager`, `TimerRegistry`,
  `InterfaceManager`, `EspNowMqttBridge`;
- **`saspik-core/drivers`**: готовые драйверы периферии (`Dht22Sensor` и
  будущие). Это пополняемая библиотека — новые переиспользуемые драйверы
  добавляй сюда;
- **`esp32-boilerplate`**: `GatewayApp` / `SensorApp` / `DeviceApp`
  (composition roots), `ChannelBank`;
- **`esp32-boilerplate/src/app/examples`**: `FloatSensor`, `RelayActuator` —
  только если драйвера нет в core. Постоянные/переиспользуемые драйверы держи
  в `saspik-core/drivers`, а не в examples.

Если класс нужен нескольким прошивкам — вынеси его в `saspik-core` отдельным
решением, не плоди копии.

- **WiFi+MQTT (`wq`)** — `GatewayApp` boilerplate (WiFi+MQTT+ESP-NOW+свои каналы)
  или новый role-шаблон в boilerplate, если готовой роли не хватает. Не делай
  standalone-прошивку в стиле `esp32-local-mqtt`.
- **ESP-NOW (`en`)** — `SensorApp`/`DeviceApp` boilerplate (`-DDEVICE_ROLE`) или
  ближайший boilerplate-role; не копируй старые скетчи.
- `platformio.ini`: env, `lib_extra_dirs = ../shared` и
  `../esp32-boilerplate/lib`, `lib_ldf_mode = deep+`, `-Iinclude`,
  `board_build.partitions = partitions.csv`.
- `extra_script.py`/`.env` — только если это осознанно нужно; по умолчанию
  дефолты в `config.h` + captive portal/NVS (как в boilerplate).
- `include/config.h` — суперсет конфига boilerplate (ядро компилируется против него).

## 4. Структура

```
<название>/
├── platformio.ini
├── partitions.csv         # 4MB, раздел "spiffs" (из boilerplate)
├── include/
│   └── config.h           # UNIT_ID, OBJECT_ID, CHANNELS[], NODE_TABLE, пины, таймауты
└── src/
    ├── main.cpp           # создание IApp (GatewayApp/SensorApp/DeviceApp)
    └── app/               # composition root и примеры из boilerplate, если нужны
```

Драйверы периферии — в `saspik-core/{include,src}/drivers/<класс>/<Модуль>`;
скетч подключает их, а не копирует.

Обязательно в `config.h`:

- `UNIT_ID`, `OBJECT_ID`, `CHANNELS[]`, `NODE_TABLE`;
- `DEVICE_ROLE` — build-флаг `-DDEVICE_ROLE=...`, если прошивка поддерживает
  несколько ролей; для dedicated-прошивки допустимо зафиксировать один `IApp`
  в `main.cpp`;
- таймауты и интервалы boilerplate: `MQTT_REBOOT_TIMEOUT_MS`,
  `WIFI_REBOOT_TIMEOUT_MS`, `SENSOR_INTERVAL_MS` и т.д.

Обязательно в коде:

- использовать `MqttManager`/`Logger` boilerplate: LWT, лог-конверт,
  неблокирующий MQTT-connect и авто-ребуты уже реализованы там — не дублируй;
- retained-статус — JSON `{"status":"online"|"offline"}`; `MqttManager` из
  core уже публикует его так. Не публикуй голые строки — сервер их не парсит;
- `main.cpp` создаёт `IApp`; логика — в composition root (`src/app/`);
- каналы — через `ChannelBank`, датчики — через `ISensor`/`Dht22Sensor` и т.п.,
  исполнители — через `IActuator`, если готовой реализации хватает (иначе —
  минимальная реализация интерфейса).

## 5. README скетча

По скиллу `docs-template` (заголовок `⧫`, крошка `[↑ Saspik-iot](../../README.md)`): описание, таблица параметров `config.h`, сборка/прошивка (`pio run -e <env>`, `pio run -t upload`), структура.

## 6. Регистрация

- Строка в таблице `saspik-iot/devices.md`: `` `objectId` `` | прошивка | тип (роль) | краткое описание.
- Пункт со ссылкой в `saspik-iot/saspik-iot-sketches/README.md` (секция «Что внутри?»).
- `devices.md` намеренно лежит вне `docs/` — в `docs.html` не краулится, не переноси.

## 7. Верификация

- `pio run -e <env>` — сборка успешна (из папки скетча).
- Проверь базу: прошивка собрана из `esp32-boilerplate`, а не копия
  `esp32-local-mqtt`; готовые классы `saspik-core` переиспользованы, а не
  продублированы.
- Проверь модули: для каждого датчика/исполнителя выбран готовый драйвер
  `saspik-core/drivers`; если драйвер новый и переиспользуемый — он добавлен
  в core, а не оставлен в скетче или `examples`.
- Проверь retained-статус: payload — JSON `{"status":"online"|"offline"}`, а не
  голая строка; иначе сервер молча игнорирует статус и UI остаётся offline.
- Сверка `grep`: `OBJECT_ID`/`UNIT_ID`, ключи `CHANNELS[]` и `NODE_TABLE`
  согласованы в `config.h`, README и `devices.md`.
- Новые топики — в `saspik-cluster/docs/mqtt-topics.md`; новый домен — ещё и в
  `mqtt/mosquitto.acl` + `telegraf.conf` (пересборка образа mosquitto).
- Итог — в `saspik-iot/IN_PROGRESS.md` (скилл `in-progress`).

## Ограничения

- Не выдавай прошивку без сборки, если PlatformIO доступен; недоступен — скажи
  прямо и отметь, что проверка на железе остаётся пользователю.
- Не дублируй существующий objectId: сначала проверь реестр и `config.h`
  соседних скетчей.
- Не копируй `esp32-local-mqtt` или другой старый скетч как основу — только
  `esp32-boilerplate`.
- Не дублируй реализации `saspik-core`: сначала core, затем
  `src/app/examples`, и только потом новый код в скетче.
- Не оставляй переиспользуемый драйвер в `src/app/examples` — его место в
  `saspik-core/drivers`.
