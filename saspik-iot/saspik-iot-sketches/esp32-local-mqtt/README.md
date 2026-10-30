[↑ Saspik-iot-sketches](../README.md)

# ⧫ esp32-local-mqtt

#### ESP32 (DOIT DevKit V1) + MQTT: конфигурируемые сенсоры и автоматы

Скетч публикует данные сенсоров и управляет автоматами по MQTT. Список каналов
задаётся в [`src/config.h`](src/config.h): имя, пин, роль и модель. Один объект
может содержать несколько автоматов и/или сенсоров. Старый объект
`saspik.sa.wm.m003` остаётся на своей прошивке — этот скетч создаёт новый объект
`saspik-awqm001`.

### Возможности

- Произвольные имена каналов (UTF-8), без хардкода `state`/`relay1`/`relay2`
- Несколько каналов на одном `objectId`; имя передаётся в JSON команды
- Модели каналов: `Dht22` (два ключа), `Float` (дискретный вход), `Relay`/`Led`
- Публикация сенсоров: JSON-карта в `sensor/unitId3/saspik-awqm001`
- Управление: `device/unitId3/saspik-awqm001/command` с payload
  `{"channel":"свет","value":"ON"}`
- **Retained-состояние**: карта `{"свет":"ON","полив":"OFF"}` в
  `device/unitId3/saspik-awqm001/state` при подключении и каждой смене
- **Retained-статус**: `{"status":"online"}` при подключении, `offline` — LWT
  брокера в `device/unitId3/saspik-awqm001/status`
- Captive portal для настройки Wi-Fi/MQTT (`wifi-config`), кнопка на GPIO 25
- **Восстановление Wi-Fi**: `WiFi.disconnect()+WiFi.reconnect()` раз в
  `WIFI_RECONNECT_INTERVAL_MS`
- **Авто-ребут**: при непрерывной недоступности MQTT >
  `MQTT_REBOOT_TIMEOUT_MS` или Wi-Fi > `WIFI_REBOOT_TIMEOUT_MS`
- **Энергонезависимая диагностика** (`device-log`): причина ребута и
  длительность прошлой сессии (`prevUptime`) в NVS
- **Удалённая диагностика**: конверт `startup` с `cause`/`prevUptime` и событие
  `reconnect` с `downSec`/`errors`

### Каналы (config.h)

```cpp
constexpr ChannelConfig CHANNELS[] = {
    {"свет",  nullptr, 13, ChannelRole::Actuator, ChannelModel::Relay, false, false},
    {"полив", nullptr, 26, ChannelRole::Actuator, ChannelModel::Relay, false, false},
};
```

| Поле | Описание |
| :--- | :--- |
| `key` | Имя канала; должно совпадать с `spec.key` объекта в UI |
| `key2` | Второй ключ DHT22 (например, `влажность`), иначе `nullptr` |
| `pin` | Пользовательский GPIO |
| `role` | `Sensor` или `Actuator` |
| `model` | `Dht22`, `Float`, `Relay`, `Led` |
| `activeLow` | Для LED/реле: `LOW` = включено |
| `mock` | Для сенсоров: генерировать значения без железа |

Пользовательские пины: `2`, `4`, `13`, `26` (и другие свободные GPIO). `25`
занят кнопкой captive portal. Автоматического discovery нет: ключ в UI должен
совпадать с именем в `config.h`.

### Протокол

| Направление | Топик | Payload |
| :--- | :--- | :--- |
| Команда | `device/unitId3/saspik-awqm001/command` | `{"channel":"свет","value":"ON"}` |
| Состояние | `device/unitId3/saspik-awqm001/state` retained | `{"свет":"ON","полив":"OFF"}` |
| Сенсоры | `sensor/unitId3/saspik-awqm001` | `{"температура":23.4,"влажность":45.1}` |
| Статус | `device/unitId3/saspik-awqm001/status` retained | `{"status":"online"}` |
| Диагностика | `device/unitId3/saspik-awqm001/log` | JSON-конверт |

`value` принимает `ON`/`OFF` (регистр не важен), а также `1`/`0` и
`TRUE`/`FALSE`. Команда с неизвестным `channel` игнорируется с записью в Serial.

### Объект в UI

Создайте объект типа `device` с ID `saspik-awqm001-device` (сервер сам добавит
постфикс, в топик уйдёт база `saspik-awqm001`) и добавьте замеры с ключами
`свет` и `полив` (модель `relay`). Ключи должны совпадать с `CHANNELS[].key`.
Тип `device` нужен, чтобы UI показывал кнопки управления.

### Параметры (config.h)

| Параметр | Значение | Описание |
| :--- | :--- | :--- |
| `OBJECT_ID` / `UNIT_ID` / `OBJECT_TYPE` | `saspik-awqm001` / `unitId3` / `sensor` | Идентификация объекта |
| `CHANNELS[]` | два реле на GPIO 13/26 | Список каналов |
| `CONFIG_BUTTON_PIN` | `25` | Кнопка captive portal |
| `MQTT_BROKER` / `MQTT_PORT` / `MQTT_USER` / `MQTT_PASS` | — | Параметры брокера |
| `TOPIC_COMMAND` | `device/unitId3/saspik-awqm001/command` | Команды автоматам |
| `TOPIC_STATE` | `device/unitId3/saspik-awqm001/state` | Retained-состояние |
| `TOPIC_STATUS` | `device/unitId3/saspik-awqm001/status` | Retained-статус |
| `TOPIC_DIAG` | `device/unitId3/saspik-awqm001/log` | Лог и диагностика |
| `MQTT_REBOOT_TIMEOUT_MS` | `900000` (15 мин) | MQTT недоступен → ребут |
| `WIFI_REBOOT_TIMEOUT_MS` | `300000` (5 мин) | Wi-Fi не поднялся → ребут |
| `WIFI_RECONNECT_INTERVAL_MS` | `10000` | Интервал попыток Wi-Fi |
| `MQTT_RECONNECT_INTERVAL_MS` | `3000` | Интервал попыток MQTT |
| `MQTT_CONNECT_TIMEOUT_MS` | `4000` | Таймаут TCP-соединения |
| `SENSOR_INTERVAL_MS` | `2000` | Период чтения сенсоров |
| `DHT_TYPE` | `22` | Тип DHT |

> Значения по умолчанию (SSID/пароль Wi-Fi) берутся из `env_config.h`, который
> генерируется `extra_script.py` из переменных окружения `.env`.

### Диагностика

Топик `TOPIC_DIAG` (`device/unitId3/saspik-awqm001/log`):

- **При старте** (если был зафиксирован авто-ребут): конверт `startup` с причиной
  и длительностью прошлой сессии; после отправки `cause`/`prevUptime` стираются
  из NVS:

```json
{"level":"info","src":"device","event":"startup","msg":"startup after reboot","uptime":5,"cause":"mqtt-timeout","prevUptime":923}
```

- **После восстановления MQTT** после обрыва: событие `reconnect`:

```json
{"level":"info","src":"device","event":"reconnect","msg":"mqtt reconnected","uptime":1000,"downSec":45,"errors":15}
```

Причины ребута: `wifi-lost`, `mqtt-timeout`. Лог также доступен в Serial
(`pio device monitor`).

### Сборка и прошивка

```bash
pio run
pio run -t upload
pio device monitor
```

### Зависимости

- `knolleary/PubSubClient`, `adafruit/DHT sensor library`, `bblanchon/ArduinoJson`
- Общие библиотеки из `../shared`: `wifi-config`, `device-log`
- Файловая система не используется: причина ребута и лог-метаданные — в NVS

### Структура

```
esp32-local-mqtt/
├── platformio.ini
├── extra_script.py       # генерация env_config.h из .env
└── src/
    ├── main.cpp
    ├── config.h          # каналы, топики, таймауты, пины
    └── env_config.h      # генерируется из .env (Wi-Fi по умолчанию)
```
