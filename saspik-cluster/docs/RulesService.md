# RulesService

   
## Формат конфигурации правил   
Конфигурация — JSON-файл с массивом правил:   
```
{
  "rules": [
    {
      "id": "unique_rule_id",
      "trigger": {
        "topic": "sensor/temperature",
        "qos": 0
      },
      "when": {
        "jsonpath": "$.value > 30"
      },
      "then": [
        {
          "action": "log",
          "params": {
            "level": "warn",
            "message": "Температура превысила порог: {{value}}°C"
          }
        },
        {
          "action": "publish",
          "params": {
            "topic": "alerts/high_temperature",
            "payload": "{\"value\": {{value}}, \"timestamp\": \"{{timestamp}}\"}",
            "qos": 1,
            "retain": false
          }
        }
      ]
    }
  ]
}

```
### Структура   
|            Поле   <br> |                                                                                Описание   <br> |
|:-----------------------|:-----------------------------------------------------------------------------------------------|
|            `id`   <br> |                                                        Уникальный идентификатор правила   <br> |
| `trigger.topic`   <br> |                                  Топик MQTT (строка или массив, поддерживает `+` и `#`)   <br> |
|   `trigger.qos`   <br> |                                                   Уровень QoS (0, 1, 2), по умолчанию 0   <br> |
|          `when`   <br> |                                               Условие срабатывания (может быть опущено)   <br> |

### Типы условий (when)   
|             Тип   <br> |                                    Параметр   <br> |                  Пример   <br> |
|:-----------------------|:---------------------------------------------------|:-------------------------------|
|    `topicRegex`   <br> |                           RegExp для топика   <br> | `"^sensor/.+/status$"`   <br> |
|      `jsonpath`   <br> |                 `$.field оператор значение`   <br> |  `"$.temperature > 30"`   <br> |
| `payloadEquals`   <br> |                   Точное совпадение payload   <br> |                  `"OK"`   <br> |
|           `and`   <br> |                          Массив условий (И)   <br> |        `[{...}, {...}]`   <br> |
|            `or`   <br> |                        Массив условий (ИЛИ)   <br> |        `[{...}, {...}]`   <br> |
|           `not`   <br> |                           Одно условие (НЕ)   <br> |                 `{...}`   <br> |

### Типы действий (then)   
**log** — логирование в консоль:   
- `level`: `info`, `warn`, `error`   
- `message`: строка с плейсхолдерами `{{fieldName}}`   
   
**publish** — публикация в топик:   
- `topic`: топик назначения   
- `payload`: строка с плейсхолдерами `{{fieldName}}`   
- `qos`: QoS (0, 1, 2)   
- `retain`: флаг retain (true/false)   
   
### Горячая перезагрузка   
Правила загружаются из `rules.json`. При изменении файла конфигурация перечитывается и подписки обновляются без остановки процесса.   
### Запуск   
```
MQTT_BROKER_URL=mqtt://localhost:1883 CONFIG_PATH=./rules.json npm start

```
