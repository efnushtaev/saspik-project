[↑ Saspik-cluster](../README.md)

# docs/mqtt-topics

Централизованное описание топиков и форматов сообщений для обмена с MQTT-брокером кластера.

## Брокер

| Параметр | Значение |
|---|---|
| Host | `185.72.145.19` |
| Port MQTT | `1883` |
| Port WebSocket | `9001` |
| Username | `admin` |
| Password | `password123` |
| Client ID | произвольный уникальный |

### Локальное окружение (docker-compose)

Локальный брокер (mosquitto) поднимается в docker-compose и отличается от продакшена:

| Параметр | Значение |
|---|---|
| Host | `localhost` (внутри docker — `mosquitto`) |
| Port MQTT | `1883` |
| Port WebSocket | `9001` |
| Аутентификация | `allow_anonymous false`, `passwordfile` |
| ACL | `acl_file` mosquitto.acl |

## Топики публикации (device → broker)

Устройства публикуют показания сенсоров в следующие топики:

### `sensor/{unitId}/dht22`

Температура и влажность воздуха.

```json
{
  "temperature": 24.5,
  "humidity": 65.0,
  "timestamp": "2026-07-26T12:00:00Z"
}
```

| Поле | Тип | Описание |
|---|---|---|
| `temperature` | number | Температура, °C |
| `humidity` | number | Влажность, % |
| `timestamp` | string | ISO 8601 |

### `sensor/{unitId}/float-1`

Поплавковый датчик уровня воды (для генератора влажности).

```json
{
  "floatSensor": 0,
  "timestamp": "2026-07-26T12:00:00Z"
}
```

| Поле | Тип | Описание |
|---|---|---|
| `floatSensor` | number | `0` — воды нет, `1` — вода есть |
| `timestamp` | string | ISO 8601 |

## Топики команд (broker → device)

Управление нагрузками через реле.

### `units/{unitId}/commands/a_relay1`

Свет (GPIO 27).

### `units/{unitId}/commands/a_relay2`

Увлажнитель (GPIO 13).

### `units/{unitId}/commands/a_relay3`

Вентилятор (GPIO 12).

### `units/{unitId}/commands/a_relay4`

Полив / клапан (GPIO 14).

### Формат команды

Поддерживаются два формата:

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

## Агрегированные данные (опционально)

### `units/{unitId}/sensors`

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

## Служебные топики

### `healthcheck/ping`

Docker healthcheck контейнера mosquitto (публикуется каждые 30 с).
Payload: `{"payload":"test"}` (валидный JSON — ранее был текст `test`, из-за которого telegraf, подписанный на `healthcheck/#` как на JSON, ронял батч `mqtt_consumer`; топик убран из JSON-входа telegraf).

### `led/control`

Управление светодиодным индикатором.
Payload: `"ON"` / `"OFF"`

## Логирование и диагностика (единый конверт)

Все компоненты (device, server, rule-engine) публикуют логи в **едином JSON-конверте** в свои лог-топики. Telegraf отводит их в отдельный measurement `logs` в InfluxDB (бакет `logs`, retention 7 дней).

### Лог-топики

| Топик | Источник |
|---|---|
| `device/{unitId}/{objectId}/log` | ESP32 (device-log): старт/ребут, connect/disconnect, диагностика, хвост лога |
| `server/worker/log` | Backend (Express): connect/disconnect/ошибки MQTT |
| `rule-engine/worker/log` | MQTT Rule Engine: срабатывание/ошибки правил, connect/disconnect |

### Схема конверта

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

### Хранение в InfluxDB

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

### Против спама

- Mosquitto: `log_type notice` (без поштучного debug).
- Rule Engine: не логирует каждое входящее сообщение — только срабатывания и ошибки правил.
- Device: пишет только события (старт/ребут/connect/диагностика/обрыв), а не каждое чтение сенсора.

## Модель ObjectItem (клиент)

```typescript
interface ObjectItem {
  id: string;
  name: string;
  type: 'sensor' | 'device';
  spec: {
    key: string;
    value?: string | number | boolean | null;
    spec: {
      model: string;
      unit?: string;
    };
  }[];
  description?: string;
}
```

`key` — идентификатор канала внутри устройства (например `"temperature"`, `"humidity"`, `"state"`).

## Правила ACL (mosquitto.acl)

```
pattern readwrite healthcheck/#
pattern readwrite clients/%c/#
pattern readwrite sensor/#
pattern readwrite led/#
pattern readwrite units/#
pattern readwrite device/+/+/log
pattern readwrite server/+/log
pattern readwrite rule-engine/+/log
```

- `healthcheck/#` — healthcheck
- `clients/%c/#` — пространство имён клиента (по Client ID)
- `sensor/#` — сенсорные топики (`sensor/{unitId}/{objectId}`)
- `led/#` — управление LED
- `units/#` — топики команд (`units/{unitId}/commands/...`)
- `device/+/+/log`, `server/+/log`, `rule-engine/+/log` — логи-конверты

## Полный цикл данных

```
ESP-NOW node                ESP32 Controller               Mosquitto               Telegraf/InfluxDB
    │                              │                          │                          │
    │── binary(CRC8) ─────────────>│                          │                          │
    │    [temp, hum, float, ts]    │                          │                          │
    │                              │── JSON sensor/{unitId}/dht22 ─>│───── all topics (#) ────>│
    │                              │── JSON sensor/{unitId}/float-1 ─>│                         │
    │                              │                          │                          │
    │                              │<── "1"/"0" ─────────────│                          │
    │                              │    units/.../a_relay1-4  │                          │
    │                              │                          │                          │
    │                              │    GPIO → relay ON/OFF   │                          │
```

## История изменений

| Дата | Автор | Изменение |
|---|---|---|
| 2026-07-26 | | Начальная версия. Описаны топики сенсоров и команд |
| 2026-08-05 | | Топики сенсоров переведены на паттерн `sensor/{unitId}/{objectId}` (вместо `sensors/...`) |
| 2026-09-03 | | Добавлено логирование: единый JSON-конверт, лог-топики device/server/rule-engine, measurement `logs`, ACL для лог-топиков |
