---
name: firmware-gen
description: Создаёт новый скетч IoT-устройства в saspik-iot-sketches — objectId по классификатору, структура на базе esp32-boilerplate, README по шаблону ⧫, регистрация в devices.md и сборка pio run.
whenToUse: Запрос «создай прошивку», «создай скетч», «добавь iot-устройство».
---

# Firmware Generator

Новый скетч в `saspik-iot/saspik-iot-sketches/<название>/`.

## 1. Параметры устройства

Уточни, если не указано в запросе:

- **Роль**: `g` (gateway) / `s` (sensor) / `a` (actuator), можно комбинировать.
- **Протокол**: `wq` (WiFi+MQTT) / `en` (ESP-NOW) / `lr` (LoRa).
- **Питание**: `m` (mains) / `b` (battery).
- **Название папки** и краткое описание для таблицы и README.
- **Железо**: пины, периферия — для `config.h` и README.

## 2. objectId и unitId

- Формат `saspik-{роли}{протокол}{питание}{nnn}` по [`docs/classification.md`](../../../saspik-iot/docs/classification.md). Источник кодов — секция «Формат»; заголовок файла с точками устарел, действует дефисный формат.
- `nnn`: `000` — шаблон, `001+` — серийные.
- Занятость проверь по `saspik-iot/devices.md` и `grep` по `OBJECT_ID` в `saspik-iot-sketches/**/config.h`.
- `unitId` — свободный (`unitId1`, `unitId2`, …); занятые — в `devices.md`, плюс учти планы из `saspik-iot/IN_PROGRESS.md` (заявленные, но ещё не созданные устройства).

## 3. База

- **WiFi+MQTT (`wq`)** — на базе `esp32-boilerplate` (`saspik-core`, DI, captive portal, MQTT, watchdog):
  - `platformio.ini`: отдельный env, `lib_extra_dirs = ../shared` и `../esp32-boilerplate/lib` (`multiple = true` для PIO ≥ 6), `lib_ldf_mode = deep+`, `-Iinclude`, `board_build.partitions = partitions.csv`, `extra_scripts = extra_script.py` (генерация `env_config.h` из `.env`);
  - `include/config.h` — суперсет конфига boilerplate (ядро компилируется против него).
- **ESP-NOW (`en`)** — ролевой шаблон boilerplate (`-DDEVICE_ROLE`) или ближайший существующий ESP-NOW-скетч как образец.
- Общие библиотеки — `../shared` (`wifi-config`, `device-log`).

## 4. Структура

```
<название>/
├── platformio.ini
├── extra_script.py        # копия из esp32-local-mqtt
├── partitions.csv         # 4MB, раздел "spiffs" (из boilerplate/local-mqtt)
└── src/
    ├── config.h           # OBJECT_ID, UNIT_ID, топики, таймауты, пины
    └── main.cpp
```

Обязательно в `config.h`:

- `OBJECT_ID`, `UNIT_ID`, `OBJECT_TYPE` (префикс data-топика);
- топики `device/<unitId>/<objectId>/command`, `.../state`, `.../status`, `.../log`;
- таймауты и интервалы как в boilerplate/local-mqtt: `MQTT_REBOOT_TIMEOUT_MS`, `WIFI_REBOOT_TIMEOUT_MS`, интервалы попыток.

Обязательно в коде:

- LWT: retained `{"status":"offline"}` при connect, `{"status":"online"}` после подключения;
- лог-конверт `{level,src,event,msg,topic,unitId,objectId,uptime[,cause]}` — telegraf подхватывает подпиской `device/+/+/log` без правок;
- неблокирующий MQTT-connect и авто-ребуты при недоступности брокера (как в boilerplate).

## 5. README скетча

По скиллу `docs-template` (заголовок `⧫`, крошка `[↑ Saspik-iot-sketches](../README.md)`): описание, таблица параметров `config.h`, сборка/прошивка (`pio run -e <env>`, `pio run -t upload`), структура.

## 6. Регистрация

- Строка в таблице `saspik-iot/devices.md`: `` `objectId` `` | прошивка | тип (роль) | краткое описание.
- Пункт со ссылкой в `saspik-iot/saspik-iot-sketches/README.md` (секция «Что внутри?»).
- `devices.md` намеренно лежит вне `docs/` — в `docs.html` не краулится, не переноси.

## 7. Верификация

- `pio run -e <env>` — сборка успешна (из папки скетча).
- Сверка `grep`: `objectId` и топики согласованы в `config.h`, README, `devices.md`.
- Новые топики — в `saspik-cluster/docs/mqtt-topics.md`; новый домен — ещё и в `mqtt/mosquitto.acl` + `telegraf.conf` (пересборка образа mosquitto).
- Итог — в `saspik-iot/IN_PROGRESS.md` (скилл `in-progress`).

## Ограничения

- Не выдавай прошивку без сборки, если PlatformIO доступен; недоступен — скажи прямо и отметь, что проверка на железе остаётся пользователю.
- Не дублируй существующий objectId: сначала проверь реестр и `config.h` соседних скетчей.
