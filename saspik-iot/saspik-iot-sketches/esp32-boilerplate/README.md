# Saspik ESP32 Boilerplate

SOLID-база для прошивок ESP32 (Arduino framework, PlatformIO) с тремя шаблонами:
**Gateway** (шлюз), **Sensor** (сенсор), **Device** (устройство). Переиспользуемая
логика вынесена в библиотеку `lib/saspik-core`; шаблоны — композиционные корни
(`src/app/`), которые собирают граф объектов через DI.

Архитектура: [`saspik-iot/docs/boilerplate-architecture.md`](../../docs/boilerplate-architecture.md).

## Возможности

| Возможность | Gateway | Sensor | Device |
| :--- | :---: | :---: | :---: |
| Captive portal (WiFi/MQTT настройка) | ✅ | ✅ | ✅ |
| Wi-Fi STA + reconnect + авто-ребут | ✅ | — | — |
| MQTT connect/sub/pub + LWT + авто-ребут | ✅ | — | — |
| ESP-NOW rx/tx | ✅ | ✅ (tx) | ✅ |
| Мост ESP-NOW ↔ MQTT | ✅ | — | — |
| Лог: UART + MQTT | ✅ | — | — |
| Лог: UART + ESP-NOW (через шлюз) | — | ✅ | ✅ |
| Light sleep по таймеру | — | ✅ | — |
| Шины I2C / OneWire / SPI / UART | ✅* | ✅ | ✅ |
| Каналы `CHANNELS[]` (сенсоры/автоматы) | ✅ | ✅ | ✅ |

Каналы задаются в `include/config.h`: имя, пин, роль и модель. Один объект может
содержать несколько автоматов и сенсоров; имена произвольные (UTF-8).

## Требования

- [PlatformIO Core](https://platformio.org/install) (≥ 6.x)
- Плата ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3

## Сборка

Шаблон выбирается build-флагом `-DDEVICE_ROLE` (см. `include/config.h`):
`1` = Gateway, `2` = Sensor, `3` = Device. Готовые env — в `platformio.ini`:

```bash
# Шлюз на базовой ESP32
pio run -e gateway_esp32

# Сенсор на ESP32-S3
pio run -e sensor_s3

# Устройство на ESP32-C3
pio run -e device_c3
```

Доступные env: `{gateway,sensor,device}_{esp32,s2,s3,c3}`.

## Загрузка прошивки

```bash
pio run -e gateway_esp32 -t upload
```

Скорость загрузки зафиксирована на 115200 (на 460800 срывается верификация
CP2102 — урок из esp32-local-mqtt).

## Монитор

```bash
pio device monitor -e gateway_esp32
```

Формат лога в терминале: `[уровень][событие] текст (аптайм, с)`.
В MQTT/ESP-NOW уходит единый JSON-конверт
`{level, src, event, msg, topic, unitId, objectId, uptime[, cause]}`.

## Первый запуск (настройка через captive portal)

1. Прошейте устройство (см. выше).
2. При отсутствии конфига в NVS (или зажатой кнопке `PIN_GPIO_2` при старте)
   устройство поднимает точку доступа с порталом.
3. Подключитесь к AP, откройте `http://192.168.4.1`, заполните WiFi/MQTT.
4. `POST /save` сохраняет конфиг в NVS и перезагружает устройство.

Дефолты формы — из `include/config.h` (`WIFI_SSID`, `MQTT_BROKER` и т.д.).

## Структура проекта

```
esp32-boilerplate/
├── platformio.ini          # env: gateway/sensor/device × esp32/s2/s3/c3 + native
├── partitions.csv          # 4MB, раздел "spiffs" (LittleFS — для будущего OTA)
├── include/config.h        # ВСЕ compile-time константы (роль, пины, шины, MQTT, ESP-NOW)
├── src/
│   ├── main.cpp            # выбор IApp по DEVICE_ROLE, yield(), loop
│   └── app/
│       ├── IApp.h          # контракт шаблона: begin()/update()
│       ├── GatewayApp.*    # Composition Root шлюза
│       ├── SensorApp.*     # Composition Root сенсора
│       ├── DeviceApp.*     # Composition Root устройства
│       └── examples/       # ChannelBank (CHANNELS[]), примеры ISensor/IActuator
├── lib/saspik-core/        # переиспользуемая SOLID-библиотека
│   ├── include/saspik/     #   core, config, log, net, wifi, portal, mqtt,
│   │                       #   espnow, bridge, sys, interfaces
│   └── src/                #   реализации (firmware-only — под #ifdef ARDUINO)
└── test/
    ├── mocks/              # моки интерфейсов (MockMqttClient, MockEspNowClient,
    │                       #   MockWifiManager, MockConfigStore, MockLogSink)
    ├── test_topic_builder/ # host-тесты (Unity): по одной папке test_* на сьют
    ├── test_esp_now_codec/
    ├── test_bridge/
    ├── test_config_manager/
    └── test_timer_registry/
```

> Каждый сьют — отдельная папка с префиксом `test_` (требование PlatformIO:
> папка = независимое тестовое приложение со своим `main()`).

## Host-тесты

Тестируются чистые модули (без железа): топики, ESP-NOW кодек (включая
known-answer CRC-8/MAXIM), мост, merge-конфиг, таймеры.

```bash
pio test -e native
```

## Конфигурация

- **Compile-time** — `include/config.h`: роль, unitId/objectId, каналы
  (`CHANNELS[]`: имя, пин, роль, модель), шины (`BUS_*_ENABLED`/`PIN_*`),
  MQTT-дефолты и таймауты, ESP-NOW (канал, типы, словарь полей, таблица
  узлов), интервалы, логирование.
- **Runtime** — NVS: WiFi/MQTT через captive portal (namespace `saspik`,
  shared/wifi-config), параметры приложения (интервалы, канал, пиры) через
  `ConfigManager` (namespace `saspik_app`). Правило merge: defaults ← NVS
  (NVS побеждает).

## Протокол ESP-NOW (v2)

```
[type(1)] [timestamp LE(4)] [fieldId(1)+len(1)+data]* [crc8(1)]
```

- Типы: 1=DATA, 2=LOG, 3=COMMAND, 4=ACK, 5=STATE.
- Служебные поля: `0xFF` — JSON лога, `0xFE` — JSON команды,
  `0xFD` — JSON данных, `0xFC` — JSON состояния.
- Данные и состояние передаются JSON-картой с произвольными именами каналов:
  `{"свет":1}` и `{"свет":"ON","полив":"OFF"}`; legacy float-поля
  (`FIELD_DICTIONARY`) поддерживаются для совместимости.
- CRC-8/MAXIM (полином 0x31 отражённый → 0x8C, init 0x00).
- Лимит пакета — 250 байт (`ESP_NOW_PACKET_MAX`).
- Несовместим с v1 (node-sensor-1); миграция узлов — отдельная задача.

## Каналы и команды

```cpp
constexpr ChannelConfig CHANNELS[] = {
    {"свет", nullptr, 13, ChannelRole::Actuator, ChannelModel::Relay, false, false},
    {"полив", nullptr, 26, ChannelRole::Actuator, ChannelModel::Relay, false, false},
};
```

- `key` — имя канала; должно совпадать с `spec.key` объекта в UI.
- `key2` — второй ключ DHT22 (например, `влажность`), иначе `nullptr`.
- `role` — `Sensor` или `Actuator`; `model` — `Dht22`, `Float`, `Relay`, `Led`.
- `activeLow` — для LED/реле; `mock` — для сенсоров без железа.

Топики:

| Направление | Топик | Payload |
| :--- | :--- | :--- |
| Команда | `device/{unitId}/{objectId}/command` | `{"channel":"свет","value":"ON"}` |
| Состояние | `device/{unitId}/{objectId}/state` retained | `{"свет":"ON","полив":"OFF"}` |
| Данные | `sensor/{unitId}/{objectId}` | `{"температура":23.4}` |

Шлюз подписан на `device/{unitId}/+/command`, упаковывает JSON в ESP-NOW
COMMAND и не разбирает имена каналов. Узел после команды отправляет STATE;
мост публикует его retained в MQTT.

## Лицензия

MIT (см. `LICENSE`). Все файлы — с SPDX-заголовком.