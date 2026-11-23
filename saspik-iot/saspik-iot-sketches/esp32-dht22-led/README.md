[↑ Saspik-iot](../../README.md)

# ⧫ esp32-dht22-led

#### Шлюз unitId2 на GatewayApp: DHT22 как сенсор и встроенный светодиод как автомат

Прошивка собрана на базе [`esp32-boilerplate`](../esp32-boilerplate/README.md)
(`saspik-core`, шаблон `GatewayApp`): WiFi + MQTT + ESP-NOW-мост + собственные
каналы. Это единственный шлюз своего юнита, поэтому wildcard-подписка на
команды ESP-NOW-узлов не дублируется. Второй экземпляр с тем же `UNIT_ID`
создаст дублирующую подписку — в юните должен работать один шлюз.

### Роли устройства

| Роль | Что делает |
| :--- | :--- |
| Шлюз | принимает команды `device/unitId2/+/command`, маршрутизирует их узлам из `NODE_TABLE` по ESP-NOW, публикует данные узлов в MQTT |
| Сенсор | DHT22 на GPIO 4 → `temp` и `humi` в `sensor/unitId2/saspik-sawqm001` |
| Автомат | встроенный светодиод GPIO 2 (`led`) по команде `device/unitId2/saspik-sawqm001/command` |

`OBJECT_ID` оставлен `saspik-sawqm001` по решению владельца; фактически устройство
совмещает роли шлюза, сенсора и актуатора.

### Каналы (include/config.h)

```cpp
constexpr ChannelConfig CHANNELS[] = {
    {"temp", "humi", 4, ChannelRole::Sensor, ChannelModel::Dht22, false, false},
    {"led",  nullptr, 2, ChannelRole::Actuator, ChannelModel::Led, true, false},
};
```

| Поле | Описание |
| :--- | :--- |
| `key` | Имя канала; должно совпадать с `spec.key` объекта в UI |
| `key2` | Второй ключ DHT22 (`humi`), иначе `nullptr` |
| `pin` | Пользовательский GPIO |
| `role` | `Sensor` или `Actuator` |
| `model` | `Dht22`, `Float`, `Relay`, `Led` |
| `activeLow` | Для LED/реле: `LOW` = включено |
| `mock` | Для сенсоров: генерировать значения без железа |

`ChannelBank` создаёт `Dht22Sensor` — драйвер из `saspik-core`
(`drivers/sensor`), реализующий `ISensor`, — и вызывает `read()` дважды за
интервал: `read()` чередует поля, поэтому двух вызовов достаточно, чтобы
получить и температуру, и влажность. Библиотека DHT кэширует физическое чтение
на ~2 с, поэтому датчик читается один раз, а `temp` и `humi` уходят в одном
JSON.

### Топики

| Направление | Топик | Payload |
| :--- | :--- | :--- |
| Свои данные | `sensor/unitId2/saspik-sawqm001` | `{"temp":23.4,"humi":45.1}` |
| Своя команда | `device/unitId2/saspik-sawqm001/command` | `{"channel":"led","value":"ON"}` |
| Своё состояние | `device/unitId2/saspik-sawqm001/state` retained | `{"led":"ON"}` |
| Статус | `device/unitId2/saspik-sawqm001/status` retained + LWT | `{"status":"online"}` |
| Лог | `device/unitId2/saspik-sawqm001/log` | JSON-конверт |
| Команды узлам | `device/unitId2/+/command` (подписка) | `{"channel":"...","value":"ON"}` |
| Данные узлов | `sensor/unitId2/<objectId-узла>` | JSON-карта каналов узла |
| Состояние узлов | `device/unitId2/<objectId-узла>/state` retained | JSON-карта каналов узла |

`value` принимает `ON`/`OFF` (регистр не важен), а также `1`/`0` и
`TRUE`/`FALSE`. Своя команда обрабатывается точной подпиской, команды других
`objectId` — мостом через wildcard.

### Таблица ESP-NOW-узлов

В `NODE_TABLE` (`include/config.h`) перечислены узлы юнита, которым шлюз
маршрутизирует команды и от которых принимает данные:

```cpp
constexpr NodeEntry NODE_TABLE[] = {
    {"saspik-aenm001", {0x02, 0x00, 0x00, 0x00, 0x00, 0x01}},
};
```

MAC в примере — заглушка, её нужно заменить на реальный MAC узла. Пока MAC не
совпадает, команды для узла получают warning `command to unknown node`, а его
данные не публикуются.

### Объект в UI

Создайте объект типа `device` с ID `saspik-sawqm001-device` (сервер сам добавит
постфикс, в топик уйдёт база `saspik-sawqm001`) и добавьте замеры с ключами
`temp` (модель `dht22`, ℃), `humi` (модель `dht22`, %) и `led` (модель `led`).
Ключи должны совпадать с `CHANNELS[].key`; тип `device` нужен, чтобы UI показывал
кнопки управления.

### Параметры (include/config.h)

| Параметр | Значение | Описание |
| :--- | :--- | :--- |
| `OBJECT_ID` / `UNIT_ID` | `saspik-sawqm001` / `unitId2` | Идентификация объекта |
| `CLIENT_ID_PREFIX` | `gw-` | clientId = `gw-saspik-sawqm001` |
| `CHANNELS[]` | `temp`/`humi` (DHT22, GPIO 4), `led` (GPIO 2) | Список каналов |
| `PIN_GPIO_2` | `25` | Кнопка captive portal |
| `NODE_TABLE` | `saspik-aenm001` | Узлы ESP-NOW; MAC заменить |
| `ESP_NOW_CHANNEL` | `1` | Канал ESP-NOW (должен совпадать с каналом Wi-Fi) |
| `WIFI_SSID` / `WIFI_PASS` | пусто | Настраивается через captive portal |
| `MQTT_BROKER` / `MQTT_PORT` / `MQTT_USER` / `MQTT_PASS` | — | Параметры брокера |
| `SENSOR_INTERVAL_MS` | `2000` | Период чтения DHT22 и публикации |
| `DIAG_PUBLISH_INTERVAL_MS` | `60000` | Период диагностики |
| `MQTT_REBOOT_TIMEOUT_MS` | `900000` (15 мин) | MQTT недоступен → ребут |
| `WIFI_REBOOT_TIMEOUT_MS` | `300000` (5 мин) | Wi-Fi не поднялся → ребут |
| `MQTT_ONLINE_REPUBLISH_MS` | `300000` (5 мин) | Период republish retained `online` |
| `STATUS_LED_PIN` | `PIN_GPIO_1` (GPIO 2) | Индикация связи на том же пине, что канал `led`; `-1` — выключить |
| `STATUS_LED_BLINK_MS` | `250` | Полупериод мигания индикации |
| `DHT_TYPE` | `22` | Тип DHT |

Значения по умолчанию заданы в `config.h`; Wi-Fi/MQTT настраиваются через
captive portal и хранятся в NVS. `.env`/`env_config.h` не используются.

### Поведение при сбое связи

Прошивка сама возвращается в строй и не оставляет устройство «мёртвым» без
единого сигнала наружу. Расшифровка индикации и настройка пина —
[`docs/indication.md`](../../docs/indication.md).

| Ситуация | Что происходит |
| :--- | :--- |
| Wi-Fi не поднялся за 20 с или нет конфига в NVS | поднимается AP `SASPIK-XXXXXX` (captive portal) |
| портал бездействует `WIFI_CONFIG_PORTAL_TIMEOUT_MS` (10 мин) | перезагрузка и повторная попытка STA — портал не вечный |
| MQTT не подключён (в т.ч. в портале) | светодиод мигает с периодом `STATUS_LED_BLINK_MS` |
| MQTT подключён | пин возвращается каналу `led`, retained `state` перепубликуется |
| раз в `MQTT_ONLINE_REPUBLISH_MS` | повторный retained `{"status":"online"}` |
| авто-ребут по watchdog | перед `ESP.restart()` в NVS пишется причина |

Причина ребута (`mqtt-lost` / `wifi-lost`) уходит **один раз** стартовым
конвертом `event=startup` с полем `cause` после первого успешного connect и
сразу очищается из NVS — поэтому обычное выключение питания не наследует
старую причину.

Индикация занимает тот же GPIO, что канал `led`: пока связи нет, команды всё
равно не приходят, а после подключения состояние канала восстанавливается.
Если светодиод ведёт себя инверсно, поменяй `STATUS_LED_ACTIVE_LOW` и
`activeLow` канала `led` в `CHANNELS[]` вместе.

### Сборка и прошивка

```bash
pio run
pio run -t upload
pio device monitor
```

Первая сборка скачивает `saspik-core` из `../esp32-boilerplate/lib` и общие
библиотеки из `../shared`.

### Проверка

- `pio run` — сборка успешна (RAM 13.9%, Flash 63.7%).
- Перед прошивкой заменить MAC `saspik-aenm001` в `NODE_TABLE` на реальный.
- После прошивки: настроить Wi-Fi/MQTT через captive portal, проверить
  `temp`/`humi` в `sensor/unitId2/saspik-sawqm001`, команду `led` ON/OFF,
  retained `state` после перезагрузки и маршрутизацию команды узлу
  `saspik-aenm001`.

### Структура

```
esp32-dht22-led/
├── platformio.ini
├── partitions.csv
├── include/
│   └── config.h          # каналы, топики, NODE_TABLE, таймауты, пины
└── src/
    ├── main.cpp          # создание GatewayApp
    └── app/
        ├── IApp.h
        ├── GatewayApp.h/.cpp
        └── examples/
            └── ChannelBank.h/.cpp
```

`GatewayApp` и `ChannelBank` — копии шаблонов из `esp32-boilerplate`; драйвер
`Dht22Sensor` подключается из `saspik-core` (`drivers/sensor`) как библиотека
через `lib_extra_dirs`. Это позволяет дорабатывать приложение под конкретное
устройство, не меняя boilerplate, и переиспользовать драйвер в других прошивках.
