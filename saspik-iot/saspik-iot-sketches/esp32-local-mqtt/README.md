[↑ Saspik-iot-sketches](../README.md)

# ⧫ esp32-local-mqtt

#### ESP32 (DOIT DevKit V1) + DHT22 + MQTT: локальный сенсор с устойчивостью к сбоям

Скетч публикует температуру и влажность в MQTT-брокер, управляет светодиодом
по командам. Отличается **повышенной устойчивостью**: восстановление Wi-Fi,
авто-ребут при длительных сбоях и энергонезависимый лог событий
с причиной последнего перезапуска.

### Возможности

- Чтение температуры/влажности с DHT22 (GPIO 4) с интервалом `SENSOR_INTERVAL_MS`
- Публикация JSON в MQTT-топик `sensor/unitId2/saspik.sa.wm.m002`
- Управление светодиодом (GPIO 2) по командам `{"state":"ON"}`/`{"state":"OFF"}` с подпиской `device/unitId2/saspik.sa.wm.m002`
- Captive portal для настройки Wi-Fi/MQTT (`wifi-config`), кнопка на GPIO 25
- **Восстановление Wi-Fi**: при обрыве `WiFi.disconnect()+WiFi.reconnect()` раз в `WIFI_RECONNECT_INTERVAL_MS`
- **Авто-ребут**: при непрерывной недоступности MQTT > `MQTT_REBOOT_TIMEOUT_MS` или Wi-Fi > `WIFI_REBOOT_TIMEOUT_MS`
- **Энергонезависимый лог** (`device-log`): причина ребута и длительность прошлой сессии (`prevUptime`) в NVS
- **Удалённая диагностика**: при старте — конверт `startup` с `cause`/`prevUptime`, после обрыва MQTT — событие `reconnect` с `downSec`/`errors`

### Параметры (config.h)

Все настраиваемые параметры вынесены в [`src/config.h`](src/config.h):

| Параметр | Значение | Описание |
| :--- | :--- | :--- |
| `OBJECT_ID` / `UNIT_ID` / `OBJECT_TYPE` | `saspik.sa.wm.m002` / `unitId2` / `sensor` | Идентификация объекта |
| `MQTT_BROKER` / `MQTT_PORT` / `MQTT_USER` / `MQTT_PASS` | — | Параметры брокера |
| `TOPIC_SUBSCRIBE` | `device/unitId2/saspik.sa.wm.m002` | Топик управления (LED) |
| `TOPIC_DIAG` | `device/unitId2/saspik.sa.wm.m002/log` | Топик лога и диагностики |
| `MQTT_REBOOT_TIMEOUT_MS` | `900000` (15 мин) | МQТТ недоступен → ребут |
| `WIFI_REBOOT_TIMEOUT_MS` | `300000` (5 мин) | Wi-Fi не поднялся → ребут |
| `WIFI_RECONNECT_INTERVAL_MS` | `10000` | Интервал попыток восстановления Wi-Fi |
| `MQTT_RECONNECT_INTERVAL_MS` | `3000` | Интервал попыток подключения к MQTT |
| `MQTT_CONNECT_TIMEOUT_MS` | `4000` | Таймаут TCP-соединения с брокером |
| `SENSOR_INTERVAL_MS` | `2000` | Период чтения датчика |
| `DHT_TYPE` | `22` | Тип датчика DHT |
| `PIN_DHT` / `PIN_LED` / `CONFIG_BUTTON_PIN` | `4` / `2` / `25` | Пины |

> Значения по умолчанию (SSID/пароль Wi-Fi) берутся из `env_config.h`, который генерируется `extra_script.py` из переменных окружения `.env`.

### Диагностика

Топик `TOPIC_DIAG` (`device/unitId2/saspik.sa.wm.m002/log`):

- **При старте** (если был зафиксирован авто-ребут): конверт `startup` с причиной
  и длительностью прошлой сессии; после отправки `cause`/`prevUptime` стираются
  из NVS, чтобы обычный (не авто-) перезагрузка не шлёт ложную причину:

```json
{"level":"info","src":"device","event":"startup","msg":"startup after reboot","uptime":5,"cause":"mqtt-timeout","prevUptime":923}
```

- **После восстановления MQTT** после обрыва: событие `reconnect`:

```json
{"level":"info","src":"device","event":"reconnect","msg":"mqtt reconnected","uptime":1000,"downSec":45,"errors":15}
```

Причины ребута: `wifi-lost`, `mqtt-timeout`. Лог также доступен в Serial (`pio device monitor`).

### Сборка и прошивка

```bash
# Сборка
pio run

# Прошивка
pio run -t upload

# Монитор Serial
pio device monitor
```

### Зависимости

- `knolleary/PubSubClient`, `adafruit/DHT sensor library`, `bblanchon/ArduinoJson`
- Общие библиотеки из `../shared`: `wifi-config`, `device-log`
- Файловая система: `board_build.filesystem = littlefs` (для лога)

### Структура

```
esp32-local-mqtt/
├── platformio.ini
├── extra_script.py       # генерация env_config.h из .env
└── src/
    ├── main.cpp
    └── config.h          # все параметры (топики, таймауты, пины)
```