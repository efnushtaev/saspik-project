[↑ Saspik-cluster](../README.md)

# ⧫ docs/mqtt-topics

Централизованное описание топиков и форматов сообщений для обмена с MQTT-брокером кластера.

Все топики формируются по строгой структуре сегментов: `{домен}/{unitId}/{objectId}[/суффикс]`. Тип сегмента определяется положением, значения не меняют структуру.

### Формат топика

Общий вид:

```
{домен}/{unitId}/{objectId}[/суффикс]
```

##### Домены

| Домен                              | Направление                | Пример                                   |
| :--------------------------------- | :------------------------- | :--------------------------------------- |
| `sensor/`                          | device → broker (данные)   | `sensor/unitId2/saspik.sa.wm.m002`       |
| `device/`                          | broker → device (команды)  | `device/unitId2/saspik.sa.wm.m002`       |
| `device/.../log`                   | device → broker (логи)     | `device/unitId2/saspik.sa.wm.m002/log`   |
| `units/{unitId}/commands/`         | команды реле               | `units/unitId1/commands/a_relay1`        |
| `units/{unitId}/sensors`           | агрегированные данные      | `units/unitId1/sensors`                  |
| `server/`                          | логи backend (Express)     | `server/worker/log`                      |
| `rule-engine/`                     | логи MQTT Rule Engine      | `rule-engine/worker/log`                 |
| `led/`                             | управление LED             | `led/control`                            |
| `healthcheck/`                     | healthcheck mosquitto      | `healthcheck/ping`                       |

##### Сегменты

| Сегмент     | Описание                                                      |
| :---------- | :------------------------------------------------------------ |
| `{unitId}`  | Идентификатор юнита (например `unitId1`, `unitId2`)           |
| `{objectId}`| Идентификатор объекта — сенсора или устройства (например `saspik.sa.wm.m001`) |
| `{суффикс}` | Квалификатор топика: команда (`commands/...`), логи (`/log`)  |

Объект отвечает только за свои топики: к данным применяется `sensor/`, к управлению — `device/`, к диагностике — суффикс `/log`.

### Топики публикации (device → broker)

Показания сенсоров публикуются в `sensor/{unitId}/{objectId}`. Объекты-сенсоры делятся по модели датчика; канал объекта отображается на поле payload (см. ObjectsService, `spec.key`).

#### `sensor/{unitId}/{objectId}` (температура/влажность)

Модель: DHT22, DS18B20, BMP280 и т.д.

```json
{
  "temperature": 24.5,
  "humidity": 65.0
}
```

##### Структура

| Поле         | Тип   | Описание      |
| :----------- | :---- | :------------ |
| `temperature`| number| Температура, °C |
| `humidity`   | number| Влажность, %  |

Топик формируется устройством из типа, юнита и id объекта:

```cpp
String topic = String(OBJECT_TYPE) + '/' + String(UNIT_ID) + '/' + String(OBJECT_ID);
```

#### `sensor/{unitId}/{objectId}` (поплавковый датчик)

Модель: float/поплавковый датчик уровня воды (например для генератора влажности).

```json
{
  "floatSensor": 0
}
```

##### Структура

| Поле          | Тип   | Описание                                  |
| :------------ | :---- | :---------------------------------------- |
| `floatSensor` | number| `0` — воды нет, `1` — вода есть          |

Внешний формат legacy-сидов может содержать поле `timestamp` (ISO 8601) — см. `esp32-controller`.

### Топики команд (broker → device)

#### `device/{unitId}/{objectId}`

Команды на объект-устройство (например LED-индикатор).

```json
{"state": "ON"}
{"state": "OFF"}
```

##### Структура

| Поле   | Тип    | Описание                  |
| :----- | :------| :------------------------ |
| `state`| string | `ON` — включить, `OFF` — выключить |

#### `units/{unitId}/commands/{objectId}`

Управление нагрузками через реле (объекты `a_relay1`–`a_relay4`). Реле внутри юнита адресуются через секцию `commands/`.

| Реле          | Нагрузка                     | GPIO |
| :------------ | :--------------------------- | :--- |
| `a_relay1`    | Свет                         | 27   |
| `a_relay2`    | Увлажнитель                  | 13   |
| `a_relay3`    | Вентилятор                   | 12   |
| `a_relay4`    | Полив / клапан               | 14   |

Поддерживаются два формата payload:

**JSON-строка** (основной формат, используется rule engine):

```
"1"   // включить
"0"   // выключить
```

**JSON-объект** (альтернативный, совместимость с mqtt-local):

```json
{"state": "ON"}
{"state": "OFF"}
```

##### Структура

| Формат    | Значение          | Направление                          |
| :-------- | :---------------- | :----------------------------------- |
| `"1"`/`"0"` | JSON-строка       | включить/выключить (rule engine)     |
| `{"state":"ON"/"OFF"}` | JSON-объект | включить/выключить (mqtt-local)      |

### Служебные топики

#### `healthcheck/ping`

Docker healthcheck контейнера mosquitto (публикуется каждые 30 с).

Payload: `{"payload":"test"}` (валидный JSON — ранее был текст `test`, из-за которого telegraf, подписанный на `healthcheck/#` как на JSON, ронял батч `mqtt_consumer`; топик убран из JSON-входа telegraf).

#### `led/control`

Управление светодиодным индикатором.

Payload: `"ON"` / `"OFF"`

#### `units/{unitId}/sensors`

Используется ClimateControlService для получения сводки по всем сенсорам юнита.

```json
{
  "objectsList": [
    { "sensorType": "temperature", "value": 24.5 },
    { "sensorType": "humidity", "value": 65.0 },
    { "sensorType": "float", "value": 0 }
  ]
}
```

##### Структура

| Поле          | Тип   | Описание                                          |
| :------------ | :---- | :------------------------------------------------ |
| `objectsList` | array | Массив показаний: `{ sensorType, value }` по каждому сенсору юнита |

#### `clients/{clientId}/#`

Пространство имён клиента (по Client ID, `%c` в ACL). Назначение — изолированные топики конкретного подключения.

### Логирование и диагностика (единый конверт)

Все компоненты (device, server, rule-engine) публикуют логи в **едином JSON-конверте** в свои лог-топики. Telegraf отводит их в отдельный measurement `logs` в InfluxDB (бакет `logs`, retention 7 дней).

##### Лог-топики

| Топик | Источник |
| :--- | :--- |
| `device/{unitId}/{objectId}/log` | ESP32 (device-log): старт/ребут, connect/disconnect, диагностика, хвост лога |
| `server/worker/log` | Backend (Express): connect/disconnect/ошибки MQTT |
| `rule-engine/worker/log` | MQTT Rule Engine: срабатывание/ошибки правил, connect/disconnect |

##### Схема конверта

```json
{
  "level":    "info",      // debug | info | warn | error
  "src":      "device",    // device | server | rule-engine
  "event":    "startup",   // startup|reboot|connect|disconnect|diag|rule-fired|rule-error|...
  "msg":      "человекочитаемый текст",
  "topic":    "device/unitId2/saspik.sa.wm.m001/log",
  "unitId":   "unitId2",   // device
  "objectId": "saspik.sa.wm.m001", // device
  "uptime":   12345,       // device, сек
  "cause":    "wifi-lost"  // device, при ребуте (wifi-lost | mqtt-timeout)
}
```

- **device**: при старте публикует `event=startup` с хвостом лога в `msg` и причиной ребута в `cause`; при обрыве — `event=diag` с `msg`-сводкой и полями `wifi`/`mqtt`/`lostSec`; `event=reboot` перед перезапуском.
- **rule-engine**: при срабатывании правила — `event=rule-fired`, при ошибке — `event=rule-error`.
- **server**: при connect/disconnect/ошибке брокера — `event=connect|disconnect|mqtt-error`.

##### Хранение в InfluxDB

- **Measurement**: `logs` (через `name_override="logs"` в telegraf).
- **Теги**: `topic`, `level`, `event`, `src`, `unitId`, `objectId`.
- **Поля**: `msg`, `cause`, `uptime` и др.
- **Бакет**: `logs` (`INFLUXDB_LOGS_BUCKET`), retention **7 дней** (`INFLUXDB_LOGS_RETENTION_DAYS`).
- Сенсорные данные остаются в measurement `mqtt_consumer` (бакет `mqtt`).

Пример Flux-запроса по логам уровня `error`:

```flux
from(bucket: "logs")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "logs")
  |> filter(fn: (r) => r.level == "error")
  |> filter(fn: (r) => r._field == "msg")
  |> last()
```

##### Против спама

- Mosquitto: `log_type notice` (без поштучного debug).
- Rule Engine: не логирует каждое входящее сообщение — только срабатывания и ошибки правил.
- Device: пишет только события (старт/ребут/connect/диагностика/обрыв), а не каждое чтение сенсора.

### Брокер

| Параметр | Значение |
| :--- | :--- |
| Host | `185.72.145.19` |
| Port MQTT | `1883` |
| Port WebSocket | `9001` |
| Username | `admin` |
| Password | `password123` |
| Client ID | произвольный уникальный |

#### Локальное окружение (docker-compose)

Локальный брокер (mosquitto) поднимается в docker-compose и отличается от продакшена:

| Параметр | Значение |
| :--- | :--- |
| Host | `localhost` (внутри docker — `mosquitto`) |
| Port MQTT | `1883` |
| Port WebSocket | `9001` |
| Аутентификация | `allow_anonymous false`, `passwordfile` |
| ACL | `acl_file` mosquitto.acl |

### Правила ACL (mosquitto.acl)

| Паттерн | Назначение |
| :--- | :--- |
| `healthcheck/#` | healthcheck |
| `clients/%c/#` | пространство имён клиента (по Client ID) |
| `sensor/#` | сенсорные топики (`sensor/{unitId}/{objectId}`) |
| `led/#` | управление LED |
| `units/#` | топики команд (`units/{unitId}/commands/...`) |
| `device/+/+/log` | логи-конверты device |
| `server/+/log` | логи-конверты server |
| `rule-engine/+/log` | логи-конверты rule-engine |

### Полный цикл данных

```
ESP-NOW node                ESP32 Controller               Mosquitto               Telegraf/InfluxDB
    │                              │                          │                          │
    │── binary(CRC8) ─────────────>│                          │                          │
    │    [temp, hum, float, ts]    │                          │                          │
    │                              │── JSON sensor/{unitId}/{objectId} ─>│─ all topics (#) ─>│
    │                              │                          │                          │
    │                              │<── "1"/"0" ─────────────│                          │
    │                              │    units/.../commands/a_relay1-4                     │
    │                              │                          │                          │
    │                              │    GPIO → relay ON/OFF   │                          │
```