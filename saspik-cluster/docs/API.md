[↑ Saspik-cluster](../README.md)

# ⧫ docs/API

#### HTTP API для управления Юнитами, Объектами и правилами

Базовый URL: `http://<cluster-host>:<port>/api/v1`.

Все маршруты монтируются с префиксом `/api/v1` (`API_V1_URL_PREFIX`).

### Units

- **GET /units/list**

Получить список всех юнитов

- **GET /units/:id**

Получить Юнит по ID (404, если нет)

- **POST /units**

Создать Юнит

- **PATCH /units/:id**

Обновить Юнит

```
{
  "name": "Теплица-1",
  "description": "Обновлённое описание"
}
```

- **DELETE /units/:id**

Удалить Юнит (каскадно удаляет объекты Юнита)

```
{
  "id": "unit-id-1",
  "name": "Теплица-1",
  "description": "Управление климатом"
}
```

⚐ ⚐ ⚐
Юнит в ответе содержит агрегированные из отдельных коллекций `objects` и `rules` (джойн по `unitId`):

```
{
  "id": "unit-id-1",
  "name": "Теплица-1",
  "description": "...",
  "objects": [ ObjectDTO ],
  "rules": [ RulesDTO ]
}
```

### Objects

- **POST /objects/list/:type**

Получить список Объектов по типу (sensor/device). Тип передаётся в пути (`sensor` или `device`), в теле — опциональный фильтр по Юниту:

```
{
  "unitId": "unit-id-1"
}
```

- **POST /objects/getByIds**

Получить Объекты по ID

```
{
  "id": ["sensor-id-1", "device-id-1"],
  "type": "sensor",
  "unitId": "unit-id-1"
}
```

- **POST /objects/command/:deviceId**

Отправить команду устройству

```
{
  "value": "on",
  "unitId": "unit-id-1"
}
```

- **POST /objects/getLastSensorsData**

Получить последние показания сенсоров

```
{
  "id": ["sensor-id-1", "sensor-id-2"]
}
```

- **POST /objects**

Создать Объект

- **PATCH /objects/:id**

Обновить Объект

```
{
  "unitId": "unit-id-1",
  "id": "sensor-temp-02",
  "name": "Датчик температуры",
  "type": "sensor",
  "topic": "sensor/unit-id-1/dht22",
  "spec": [ { "key": "temperature", "model": "dht22", "unit": "°C" } ]
}
```

- **DELETE /objects/:id**

Удалить Объект

⚐ ⚐ ⚐
Ответ списков Объектов (`list`, `getByIds`) обогащён текущими значениями из state store: каждый элемент `spec` превращается в `{ key, value, spec }`, где `value` — текущее показание канала (с учётом `minorPart` — округление):

```
{
  "objects": [
    {
      "id": "sensor-temp-01",
      "name": "Датчик температуры",
      "type": "sensor",
      "topic": "sensor/unit-id-1/dht22",
      "spec": [
        { "key": "temperature", "value": 24.5, "spec": { "key": "temperature", "model": "dht22", "unit": "°C" } },
        { "key": "humidity", "value": 65.0, "spec": { "key": "humidity", "model": "dht22", "unit": "%" } }
      ]
    }
  ]
}
```

### Rules

- **GET /rules?unitId=unit-id-1**

Возвращает правила юнита (движок исключает правила с `enabled: false`)

- **POST /rules**

Создать правило

- **PATCH /rules/:id**

Частичное обновление; для включения/выключения достаточно передать `{ "enabled": true|false }`

```
{
  "id": "rule-heat",
  "name": "Обогрев при низкой температуре",
  "unitId": "unit-id-1",
  "trigger": { "topic": "sensor/unit-id-1/dht22", "qos": 0 },
  "when": { "jsonpath": "$.temperature < 15" },
  "then": [ { "action": "log", "params": { "level": "warn", "message": "Низкая температура" } } ],
  "enabled": true
}
```

- **DELETE /rules/:id**

Удалить правило
