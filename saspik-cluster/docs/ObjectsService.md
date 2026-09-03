[↑ Saspik-cluster](../README.md)

# ⧫ docs/ObjectsService

Объекты хранятся в MongoDB (коллекция `objects`) и наполняются сервисом seed при старте (`SeedService`).

Файловый конфиг `objects.json` больше не используется — данные живут в БД. Объект связан с юнитом полем `unitId`.

### Формат объекта

```
{
  "id": "sensor-temp-01",
  "name": "Датчик температуры",
  "type": "sensor",
  "unitId": "unit-id-1",
  "topic": "sensor/unit-id-1/dht22",
  "description": "Датчик в теплице",
  "spec": [
    { "key": "temperature", "model": "dht22", "unit": "°C", "minorPart": 1 },
    { "key": "humidity", "model": "dht22", "unit": "%" }
  ]
}
```

##### Структура

| Поле          | Описание                                                |
| :------------ | :------------------------------------------------------ |
| `id`          | Уникальный идентификатор объекта                        |
| `name`        | Название объекта                                        |
| `type`        | Тип объекта: `sensor` или `device`                      |
| `unitId`      | ID юнита, к которому относится объект                   |
| `topic`       | MQTT-топик (JSON), из которого берутся значения каналов |
| `description` | Описание объекта                                        |
| `spec`        | Массив характеристик каналов (см. ниже)                 |

##### Элемент spec (канал)

| Поле        | Описание                                                         |
| :---------- | :--------------------------------------------------------------- |
| `key`       | Идентификатор канала внутри payload (например `temperature`)     |
| `model`     | Модель сенсора/устройства (dht22, ds18b20, bmp280, relay и т.д.) |
| `unit`      | Единица измерения (°C, %, hPa, lux, ppm)                         |
| `minorPart` | Число знаков после запятой при форматировании значения           |

Текущее значение канала (`getObjectState`) читается из MQTT state store по топику `topic` и ключу `key`; при наличии `minorPart` число форматируется с соответствующим округлением.

### DTO

Объект (Object) — JSON Schema, используемая для передачи данных между сервисами:

```
{
  "$schema": "...",
  "title": "Object",
  "type": "object",
  "definitions": {
    "specItem": {
      "type": "object",
      "properties": {
        "key": { "type": "string", "description": "Идентификатор канала внутри payload" },
        "model": { "type": "string", "description": "Модель сенсора/устройства" },
        "unit": { "type": "string", "description": "Единица измерения (°C, %, hPa, lux, ppm)" },
        "minorPart": { "type": "number", "description": "Число знаков после запятой" }
      },
      "required": ["key", "model"]
    }
  },
  "properties": {
    "id": { "type": "string", "description": "Уникальный ID объекта" },
    "name": { "type": "string", "description": "Название объекта" },
    "type": { "type": "string", "enum": ["sensor", "device"], "description": "Тип объекта" },
    "unitId": { "type": "string", "description": "ID юнита" },
    "topic": { "type": "string", "description": "MQTT-топик источника данных" },
    "description": { "type": "string", "description": "Описание объекта" },
    "spec": {
      "type": "array",
      "description": "Характеристики каналов объекта",
      "items": { "$ref": "#/definitions/specItem" }
    }
  },
  "required": ["id", "name", "type", "unitId", "topic", "spec"]
}
```
