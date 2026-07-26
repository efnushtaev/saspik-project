# UnitsService

   
## Формат конфигурации юнитов   
Юнит (Unit) — это логическая единица кластера. Конфигурация — JSON-файл с массивом юнитов:   
```
{
  "units": [
    {
      "name": "Теплица-1",
      "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
      "description": "Управление климатом в теплице-1",
      "objects": [
        {
          "id": "sensor-temp-01",
          "name": "Датчик температуры",
          "type": "sensor",
          "spec": { "model": "dht22", "unit": "°C" }
        },
        {
          "id": "dev-heater-01",
          "name": "Реле обогревателя",
          "type": "device",
          "spec": { "model": "relay" }
        }
      ],
      "rules": [
        {
          "id": "rule-heat",
          "name": "Включить обогрев при низкой температуре",
          "condition": "sensor-temp-01.temperature < 15.0",
          "action": "Включить реле dev-heater-01",
          "enabled": true
        }
      ]
    }
  ]
}

```
### Структура   
|          Поле   <br> |                                                            Описание   <br> |
|:---------------------|:---------------------------------------------------------------------------|
|        `name`   <br> |                                                      Название юнита   <br> |
|          `id`   <br> |                                      Уникальный идентификатор юнита   <br> |
| `description`   <br> |                                                      Описание юнита   <br> |
|     `objects`   <br> |                                Массив объектов (сенсоры/устройства)   <br> |
|       `rules`   <br> |                                         Массив правил автоматизации   <br> |

### Составляющие   
**object** — сенсор или устройство (см. ObjectsService):   
|     Поле   <br> |                                                 Описание   <br> |
|:----------------|:----------------------------------------------------------------|
|     `id`   <br> |                                    Уникальный ID объекта   <br> |
|   `name`   <br> |                                         Название объекта   <br> |
|   `type`   <br> |                               Тип: `sensor` или `device`   <br> |
|   `spec`   <br> |                  Характеристики (model, unit для sensor)   <br> |

**rule** — правило автоматизации:   
|        Поле   <br> |                                         Описание   <br> |
|:-------------------|:--------------------------------------------------------|
|        `id`   <br> |                           Уникальный ID сценария   <br> |
|      `name`   <br> |                                Название сценария   <br> |
| `condition`   <br> |                             Условие срабатывания   <br> |
|    `action`   <br> |                        Действие при срабатывании   <br> |
|   `enabled`   <br> |                               Включено/выключено   <br> |

### Горячая перезагрузка   
Юниты загружаются из `units.json`. При изменении файла конфигурация перечитывается и подписки обновляются без остановки процесса.   
### Запуск   
```
CONFIG_PATH=./units.json npm start

```
 --- 
   
## DTO   
Юнит (Unit) — JSON Schema, используемая для передачи данных между сервисами:   
```
{
  "$schema": "...",
  "title": "Unit",
  "type": "object",
  "definitions": {
    "object": {
      "$ref": "objects-schema.json"
    },
    "rule": {
      "type": "object",
      "properties": {
        "id": { "type": "string", "description": "Уникальный ID сценария" },
        "name": { "type": "string", "description": "Название сценария" },
        "condition": { "type": "string", "description": "Условие срабатывания" },
        "action": { "type": "string", "description": "Действие при срабатывании" },
        "enabled": { "type": "boolean", "description": "Включено/выключено" }
      },
      "required": ["id", "name", "condition", "action"]
    }
  },
  "properties": {
    "id": { "type": "string", "format": "uuid", "description": "Уникальный идентификатор юнита" },
    "name": { "type": "string", "description": "Название юнита" },
    "description": { "type": "string", "description": "Описание юнита" },
    "objects": {
      "type": "array",
      "description": "Список объектов (сенсоры/устройства)",
      "items": { "$ref": "#/definitions/object" }
    },
    "rules": {
      "type": "array",
      "description": "Список правил автоматизации",
      "items": { "$ref": "#/definitions/rule" }
    }
  },
  "required": ["id", "name", "description", "objects", "rules"]
}

```
