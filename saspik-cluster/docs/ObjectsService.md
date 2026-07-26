# ObjectsService

   
## Формат конфигурации объектов   
Объекты — это составляющие юнита. Конфигурация — JSON-файл с массивом объектов:   
```
{
  "objects": [
    {
      "id": "sensor-temp-01",
      "name": "Датчик температуры",
      "type": "sensor",
      "spec": {
        "model": "dht22",
        "unit": "°C"
      }
    },
    {
      "id": "dev-heater-01",
      "name": "Реле обогревателя",
      "type": "device",
      "spec": {
        "model": "relay"
      }
    }
  ]
}

```
### Структура   
|     Поле   <br>   <br>   <br> |                                                       Описание   <br>   <br>   <br> |
|:------------------------------|:------------------------------------------------------------------------------------|
|     `id`   <br>   <br>   <br> |                               Уникальный идентификатор объекта   <br>   <br>   <br> |
|   `name`   <br>   <br>   <br> |                                               Название объекта   <br>   <br>   <br> |
|   `type`   <br>   <br>   <br> |                             Тип объекта: `sensor` или `device`   <br>   <br>   <br> |
|   `spec`   <br>   <br>   <br> |                              Характеристики объекта (см. ниже)   <br>   <br>   <br> |

### Типы объектов   
**sensor** — сенсор/датчик:   
|     Поле   <br>   <br>   <br> |                                                       Описание   <br>   <br>   <br> |
|:------------------------------|:------------------------------------------------------------------------------------|
|  `model`   <br>   <br>   <br> |                 Модель сенсора (dht22, ds18b20, bmp280 и т.д.)   <br>   <br>   <br> |
|   `unit`   <br>   <br>   <br> |                       Единица измерения (°C, %, hPa, lux, ppm)   <br>   <br>   <br> |

**device** — устройство:   
|     Поле   <br>   <br>   <br> |                                                              Описание   <br>   <br>   <br> |
|:------------------------------|:-------------------------------------------------------------------------------------------|
|  `model`   <br>   <br>   <br> |                    Модель устройства (relay, pump, valve, fan и т.д.)   <br>   <br>   <br> |

### Горячая перезагрузка   
Объекты загружаются из `objects.json`. При изменении файла конфигурация перечитывается и подписки обновляются без остановки процесса.   
### Запуск   
```
CONFIG_PATH=./objects.json npm start

```
 --- 
   
## DTO   
Объект (Object) — JSON Schema, используемая для передачи данных между сервисами:   
```
{
  "$schema": "...",
  "title": "Object",
  "type": "object",
  "definitions": {
    "sensorSpec": {
      "type": "object",
      "properties": {
        "model": { "type": "string", "description": "Модель сенсора (dht22, ds18b20, bmp280)" },
        "unit": { "type": "string", "description": "Единица измерения (°C, %, hPa, lux, ppm)" }
      },
      "required": ["model"]
    },
    "deviceSpec": {
      "type": "object",
      "properties": {
        "model": { "type": "string", "description": "Модель устройства (relay, pump, valve, fan)" }
      },
      "required": ["model"]
    }
  },
  "properties": {
    "id": { "type": "string", "description": "Уникальный ID объекта" },
    "name": { "type": "string", "description": "Название объекта" },
    "type": { "type": "string", "enum": ["sensor", "device"], "description": "Тип объекта" },
    "spec": {
      "type": "object",
      "description": "Характеристики объекта",
      "oneOf": [
        { "$ref": "#/definitions/sensorSpec" },
        { "$ref": "#/definitions/deviceSpec" }
      ]
    }
  },
  "required": ["id", "name", "type", "spec"]
}

```
