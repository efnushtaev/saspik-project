[↑ Saspik-cluster](saspik-cluster/README.md)

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
| `sensor/`                          | device → broker (данные)   | `sensor/unitId3/saspik-awqm001`          |
| `device/.../command`               | broker → device (команды)  | `device/unitId3/saspik-awqm001/command`  |
| `device/.../state`                 | device → broker (retained) | `device/unitId3/saspik-awqm001/state`    |
| `device/.../status`                | device → broker (retained) | `device/unitId3/saspik-awqm001/status`   |
| `device/.../log`                   | device → broker (логи)     | `device/unitId3/saspik-awqm001/log`      |
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

### Нейминг объектов (постфиксы)

Идентификатор объекта в UI и БД состоит из базового идентификатора устройства и постфикса функциональной роли:

| Постфикс | Роль (тип объекта) | Пример id                        |
| :------- | :----------------- | :------------------------------- |
| `-sensor`| мониторинг (`sensor`) | `saspik.sa.wm.m003-sensor`   |
| `-device`| автоматика (`device`)  | `saspik.sa.wm.m003-device`   |

Правила:

- сегмент `{objectId}` в топике — всегда **база без постфикса**: для объекта `saspik.sa.wm.m003-sensor` топик будет `sensor/unitId3/saspik.sa.wm.m003`;
- в прошивке `OBJECT_ID` задаётся базой без постфикса — публикация данных `sensor/{unitId}/{base}` и подписка на команды `device/{unitId}/{base}` совпадают с топиком объекта из UI;
- сервер при создании объекта сам дописывает постфикс по типу и строит топик от базы, поэтому поле ID в форме содержит итоговое значение, которое уйдёт в базу;
- объекты, созданные до постфиксов (например `saspik.sa.wm.m003`), совместимы: постфикс просто отсутствует, топики выводятся от того же базового идентификатора;
- новые устройства автоматики: `saspik-awqm001` (`unitId3`, local-mqtt) и `saspik-aenm001` (`unitId2`, boilerplate); UI создаёт объект с постфиксом `-device`, в прошивке `OBJECT_ID` — база без постфикса.

### Топики публикации (device → broker)

Показания сенсоров публикуются в `sensor/{unitId}/{objectId}`. Объекты-сенсоры делятся по модели датчика; канал объекта отображается на поле payload (см. ObjectsService, `spec.key`).

#### `sensor/{unitId}/{objectId}`

Payload — JSON-карта с произвольными именами каналов. Имена задаются
пользователем в `config.h` прошивки и в UI (`spec.key`); автоматического
discovery нет.

```json
{
  "температура": 24.5,
  "влажность": 65.0,
  "свет": 1
}
```

Модель канала (DHT22, геркон и т.д.) определяет, какие ключи он публикует.
Двоичные сенсоры передаются как `0`/`1`.

Топик формируется устройством из типа, юнита и id объекта:

```cpp
String topic = String(OBJECT_TYPE) + '/' + String(UNIT_ID) + '/' + String(OBJECT_ID);
```

### Топики команд (broker → device)

#### `device/{unitId}/{objectId}/command`

Команды на объект-устройство. Один топик на объект; имя канала передаётся в
payload, поэтому на одном объекте может быть сколько угодно автоматов.

```json
{"channel": "свет", "value": "ON"}
{"channel": "полив", "value": "OFF"}
```

##### Структура

| Поле      | Тип    | Описание                          |
| :-------- | :----- | :-------------------------------- |
| `channel` | string | Имя канала (`spec.key` в UI)      |
| `value`   | string | `ON` — включить, `OFF` — выключить |

`value` также принимает `1`/`0` и `TRUE`/`FALSE`. Команда с неизвестным
`channel` игнорируется устройством. Legacy `{"state":"ON"}` без `channel`
поддерживается для старых объектов и применяется к первому автомату.

Старые объекты `saspik.sa.*` (например `m003`) сервер по-прежнему отправляет в
базовый топик `device/{unitId}/{objectId}` с payload `{"state":"ON"|"OFF"}` —
их прошивка не знает `/command`. Новые объекты (`saspik-awqm001`,
`saspik-aenm001`) используют `/command`.

### Состояние и статус устройства

#### `device/{unitId}/{objectId}/state`

Retained-состояние всех автоматов объекта. Публикуется устройством при
подключении и после каждой смены; telegraf подписан на `device/+/+/state` и
складывает все строковые поля в Influx, откуда их читает UI.

```json
{"свет": "ON", "полив": "OFF"}
```

Имена ключей совпадают с `channel` в командах и `spec.key` в UI.

#### `device/{unitId}/{objectId}/status`

Retained-статус присутствия. `online` публикуется при подключении, `offline` — LWT-сообщение брокера при обрыве соединения.

```json
{"status": "online"}
{"status": "offline"}
```

Оба топика входят в ACL-правило `device/#` вместе с командами и логом.

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
| `led/#` | управление LED |
| `units/#` | топики команд (`units/{unitId}/commands/...`) |
| `device/#` | команды, retained `state`/`status` и логи device |
| `server/+/log` | логи-конверты server |
| `rule-engine/+/log` | логи-конверты rule-engine |

> ACL вшит в образ (`mqtt/Dockerfile`). После правки `mosquitto.acl` нужен rebuild `mosquitto` (`docker compose build mosquitto && docker compose up -d mosquitto`); `restart`/`up -d` без `--build` поднимают старый ACL.
