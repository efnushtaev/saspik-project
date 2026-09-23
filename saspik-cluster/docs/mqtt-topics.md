[↑ Saspik-cluster](../README.md)

# ⧫ docs/mqtt-topics

#### Централизованное описание топиков и форматов сообщений для обмена с MQTT-брокером кластера.

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

### Топики команд (broker → device)

#### `device/{unitId}/{objectId}`

Команды на объект-устройство.

```json
{"state": "ON"}
{"state": "OFF"}
```

##### Структура

| Поле   | Тип    | Описание                  |
| :----- | :------| :------------------------ |
| `state`| string | `ON` — включить, `OFF` — выключить |

### Служебные топики

#### `healthcheck/ping`

Docker healthcheck контейнера mosquitto (публикуется каждые 30 с).

Payload: `{"payload":"test"}` (валидный JSON — ранее был текст `test`, из-за которого telegraf, подписанный на `healthcheck/#` как на JSON, ронял батч `mqtt_consumer`; топик убран из JSON-входа telegraf).

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
| `units/#` | топики команд (`units/{unitId}/commands/...`) |
| `device/+/+/log` | логи-конверты device |
| `server/+/log` | логи-конверты server |
| `rule-engine/+/log` | логи-конверты rule-engine |