# MQTT Topics & Data Formats

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

Docker healthcheck. Payload: `"test"`

### `led/control`

Управление светодиодным индикатором.
Payload: `"ON"` / `"OFF"`

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
```

- `healthcheck/#` — healthcheck
- `clients/%c/#` — пространство имён клиента (по Client ID)
- `sensor/#` — сенсорные топики (`sensor/{unitId}/{objectId}`)
- `led/#` — управление LED

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
