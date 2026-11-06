[↑ Saspik-cluster](saspik-cluster/README.md)

# ⧫ docs/API

#### HTTP API для управления Юнитами, Объектами, правилами и командами устройствам

Базовый URL: `http://<cluster-host>:<port>/api/v1`.

Все маршруты монтируются с префиксом `/api/v1` (`API_V1_URL_PREFIX`); домены — `/units`, `/objects`, `/rules`. Роутер `/mqtt` (`MqttController`) в приложение не смонтирован.

### Служебные эндпоинты

- **GET /health** (`http://<cluster-host>:<port>/health`, без префикса) — `200 OK`, текстовый ответ `OK`; используется healthcheck'ом контейнера.
- **GET /api/getTimestamp** (без префикса `/api/v1`) — `{ "timestamp": "2026-11-06T09:00:00.000Z" }`.

### Ошибки

- Успех — `2xx` с телом ответа; ошибки — JSON `{ "error": "<сообщение>" }` с HTTP-кодом (400 — невалидный запрос, 404 — не найдено, 500 — внутренняя ошибка).
- Исключение для команды: ошибка отдаётся как `{ "success": false, "error": "<сообщение>" }` с кодом 500.
- Централизованной валидации DTO нет: проверки выполняются в сервисах/контроллерах, поэтому часть некорректных запросов вернёт 400 с текстом ошибки, а не структурированный список полей.

### Units

- **GET /units/list**

Получить список всех юнитов.

- **GET /units/:id**

Получить Юнит по ID (404, если нет).

- **POST /units**

Создать Юнит.

```
{
  "id": "unit-id-1",
  "name": "Теплица-1",
  "description": "Управление климатом"
}
```

- **PATCH /units/:id**

Обновить Юнит (id не меняется).

```
{
  "name": "Теплица-1",
  "description": "Обновлённое описание"
}
```

- **DELETE /units/:id**

Удалить Юнит; каскадно удаляет объекты Юнита (правила не удаляются).

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

Получить список Объектов по типу (`sensor` или `device` передаётся в пути). В теле — опциональный фильтр по Юниту:

```
{
  "unitId": "unit-id-1"
}
```

- **POST /objects/getByIds**

Получить Объекты по ID. Фильтры `type` и `unitId` опциональны.

```
{
  "id": ["sensor-id-1", "device-id-1"],
  "type": "sensor",
  "unitId": "unit-id-1"
}
```

- **POST /objects/command/:deviceId**

Отправить команду устройству. `deviceId` — ID объекта типа `device` (можно с постфиксом `-device` и без).

```
{
  "value": "ON",
  "key": "свет",
  "unitId": "unit-id-1"
}
```

Что происходит:

| Условие | Публикация |
| :--- | :--- |
| Топик объекта начинается с `device/`, `key` указан и есть в `spec` объекта | `${topic}/command`, payload `{"channel":"<key>","value":"<value>"}` |
| `key` не найден в `spec` | Публикации нет, пишется лог `unknown channel key` |
| Legacy: топик `units/...`, объект `saspik.sa.*` или `key` не передан | Базовый топик объекта, payload `{"<specKey>":"<value>"}`, где `specKey` — `spec[0].key` или `state` |

- `value` — строка; для устройств ожидаются `ON`/`OFF` (прошивка принимает также `1`/`0`, `TRUE`/`FALSE`), для legacy-команд климата — `1`/`0`.
- `key` — имя канала, совпадающее с `spec.key` в UI и `channel` в прошивке.
- Ответ: `{ "success": true }`; при ошибке — 500 `{ "success": false, "error": "..." }`.

Подробности форматов и топиков — в [mqtt-topics](mqtt-topics.md).

- **POST /objects/getLastSensorsData**

Получить последние показания сенсоров. Сейчас — заглушка: возвращает `{}`.

```
{
  "id": ["sensor-id-1", "sensor-id-2"]
}
```

- **POST /objects**

Создать Объект. Сервер сам схлопывает постфикс `-sensor`/`-device`, дописывает постфикс по типу и строит `topic` = `${type}/${unitId}/${base}` — передавать `topic` не нужно.

```
{
  "unitId": "unit-id-1",
  "id": "saspik-awqm001-device",
  "name": "Автоматика теплицы",
  "type": "device",
  "description": "Два реле: свет, полив",
  "spec": [
    { "key": "свет", "model": "relay", "unit": "", "minorPart": 0 },
    { "key": "полив", "model": "relay", "unit": "", "minorPart": 0 }
  ]
}
```

Ответ: `{ "object": ObjectsDTO }`.

- **PATCH /objects/:id**

Обновить Объект. `id` в теле игнорируется — берётся из пути.

```
{
  "unitId": "unit-id-1",
  "id": "sensor-temp-02",
  "name": "Датчик температуры",
  "type": "sensor",
  "spec": [ { "key": "temperature", "model": "dht22", "unit": "°C" } ]
}
```

- `topic` пересчитывается только для канонических топиков `sensor|device/{unitId}/...`; произвольные топики (например `led/control`) сохраняются как есть, чтобы не рвать подписки.
- Ответ: `{ "object": ObjectsDTO }`; 404 — объект не найден.

- **DELETE /objects/:id**

Удалить Объект. В теле обязателен `unitId`:

```
{
  "unitId": "unit-id-1"
}
```

Ответ: `{ "success": true }`; 404 — объект не найден.

⚐ ⚐ ⚐
Ответ списков Объектов (`list`, `getByIds`) обогащён текущими значениями из state store: каждый элемент `spec` превращается в `{ key, value, spec }`, где `value` — последнее показание канала (учитывается `minorPart` — округление). Дополнительно у объекта есть `updatedAt` (максимальный timestamp среди каналов) и `status` (`online`/`offline` из retained `device/.../status`).

```
{
  "objects": [
    {
      "id": "sensor-temp-01",
      "name": "Датчик температуры",
      "type": "sensor",
      "topic": "sensor/unit-id-1/dht22",
      "status": "online",
      "updatedAt": "2026-11-06T09:14:03.000Z",
      "spec": [
        { "key": "temperature", "value": 24.5, "spec": { "key": "temperature", "model": "dht22", "unit": "°C" } },
        { "key": "humidity", "value": 65.0, "spec": { "key": "humidity", "model": "dht22", "unit": "%" } }
      ]
    }
  ]
}
```

- Значения читаются по `object.topic`; если топик объекта не совпадает с топиком, куда публикует устройство, каналы вернутся с `value: null`.
- `status` берётся из подписки сервера на `device/+/+/status` и присутствует только у объектов с каноническим `device/...` топиком.

### Rules

- **GET /rules?unitId=unit-id-1**

Возвращает правила юнита (без фильтра — все). Правила с `enabled: false` возвращаются, но движок их не исполняет.

- **POST /rules**

Создать правило (upsert по `id`: повторный POST с тем же `id` перезапишет правило).

- **PATCH /rules/:id**

Частичное обновление; для включения/выключения достаточно передать `{ "enabled": true|false }`.

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

Удалить правило.

⚐ ⚐ ⚐
Правила климат-контроля в `server/data/rules.json` используют подстановку `${VAR}` и `${expr:...}`; `unitId` при сиде вычисляется из топика триггера. Ограничение: движок не подставляет `unitId` из wildcard-топика, поэтому правила дублируются на каждый юнит — см. [RulesService](RulesService.md).
