[↑ Saspik-cluster](../README.md)

# ⧫ docs/RulesService

Правила хранятся в MongoDB (коллекция `rules`, поле `unitId` связывает с юнитом) и наполняются сервисом seed при старте (`SeedService`).

При записи в БД подставляются переменные окружения (`${ENV_VAR}`) и выражения (`${expr:...}`).
```
{
  "rules": [
    {
      "id": "unique_rule_id",
      "name": "Температурный порог",
      "unitId": "unit-id-1",
      "trigger": {
        "topic": "sensor/unit-id-1/dht22",
        "qos": 0
      },
      "when": {
        "jsonpath": "$.temperature > 30"
      },
      "then": [
        {
          "action": "log",
          "params": {
            "level": "warn",
            "message": "Температура превысила порог: {{temperature}}°C"
          }
        },
        {
          "action": "publish",
          "params": {
            "topic": "alerts/high_temperature",
            "payload": "{\"value\": {{temperature}}, \"timestamp\": \"{{timestamp}}\"}",
            "qos": 1,
            "retain": false
          }
        }
      ],
      "enabled": true
    }
  ]
}
```
### Структура
|Поле|Описание|
|:-----------------------|:-----------------------------------------------------------------------------------------------|
|`id`|Уникальный идентификатор правила|
|`name`|Название правила (опционально)|
|`unitId`|ID юнита, к которому относится правило|
|`trigger.topic`|Топик MQTT (строка или массив, поддерживает `+` и `#`)|
|`trigger.qos`|Уровень QoS (0, 1, 2), по умолчанию 0|
|`when`|Условие срабатывания (может быть опущено)|
|`then`|Массив действий (не пустой)|
|`enabled`|Флаг включения: правила с `enabled: false` не загружаются движком|

### Типы условий (when)
|Тип|Параметр|Пример|
|:-----------------------|:---------------------------------------------------|:-------------------------------|
|`topicRegex`|RegExp для топика|`"^sensor/.+/status$"`|
|`jsonpath`|`$.field оператор значение`|`"$.temperature > 30"`|
|`payloadEquals`|Точное совпадение payload|`"OK"`|
|`and`|Массив условий (И)|`[{...}, {...}]`|
|`or`|Массив условий (ИЛИ)|`[{...}, {...}]`|
|`not`|Одно условие (НЕ)|`{...}`|
|`timeBetween`|Временной интервал по часу (см. ниже)|`{ "start": 22, "end": 6 }`|

Операторы `jsonpath`: `==`, `!=`, `>`, `>=`, `<`, `<=`.
**timeBetween** — срабатывает, если текущий час (с учётом `MOSCOW_OFFSET_HOUR`, по умолчанию 3) попадает в интервал `[start, end)`. При `start > end` интервал считается «через полночь». Параметры: `start` (number), `end` (number), опционально `field` — путь к ISO-дате в payload (иначе используется время получения сообщения).
```
{ "when": { "timeBetween": { "start": 22, "end": 6 } } }
```

### Типы действий (then)
**log** — логирование в консоль:
- `level`: `info`, `warn`, `error`
- `message`: строка с плейсхолдерами `{{fieldName}}`

**publish** — публикация в топик:
- `topic`: топик назначения
- `payload`: строка с плейсхолдерами `{{fieldName}}`
- `qos`: QoS (0, 1, 2), по умолчанию 0
- `retain`: флаг retain (true/false), по умолчанию false

**timeout** — отложенный запуск вложенных действий:
- `delay`: задержка в миллисекундах
- `then`: массив действий, выполняемых после задержки
```
{ "action": "timeout", "params": { "delay": 5000, "then": [ { "action": "publish", "params": { "topic": "alerts/slow", "payload": "1" } } ] } }
```
Плейсхолдеры `{{fieldName}}` в `payload`/`message` заменяются значениями из JSON-контекста сообщения (доступ через jsonpath-путь `$.fieldName`).

## Источники правил
Движок получает правила через провайдер, источник задаётся переменной `RULES_SOURCE`:

| Источник | Провайдер | Поведение |
|---|---|---|
| `mongo` (по умолчанию) | `MongoRulesProvider` | Поллинг коллекции `rules` каждые 5 с (`RULES_POLL_INTERVAL_MS`), фильтр `enabled !== false` |
| `api` | `ApiRulesProvider` | Поллинг `GET RULES_API_URL` каждые 5 с, ожидает `{ "rules": [...] }`, фильтр `enabled !== false` |
| `file` | `ConfigWatcher` | Следит за файлом `CONFIG_PATH` (по умолчанию `./rules.json`), перечитывает при изменении |

Во всех режимах набор правил пересобирается только при изменении сигнатуры (полного JSON) — при неизменных данных провайдер не вызывает перезагрузку. Изменение `enabled` в БД/API автоматически применяется движком на следующем поллинге (правило исключается из обработки).

### Запуск (файловый режим)
```
MQTT_BROKER_URL=mqtt://localhost:1883 CONFIG_PATH=./rules.json npm start
```
Для MongoDB-режима: `RULES_SOURCE=mongo MONGODB_URL=mongodb://... npm start`.
