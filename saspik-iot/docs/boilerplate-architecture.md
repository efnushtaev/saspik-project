[↑ Saspik-iot](../README.md)

# ⧫ Архитектура esp32-boilerplate

#### SOLID-основа прошивок ESP32: три шаблона ролей (Gateway, Sensor, Device) на общей библиотеке saspik-core
Платформа — ESP32 / S2 / S3 / C3, framework — Arduino, сборка — PlatformIO.
Шаблон выбирается build-флагом `-DDEVICE_ROLE` (`1` = Gateway, `2` = Sensor, `3` = Device).
Переиспользуемая логика — библиотека `lib/saspik-core`; приложения-шаблоны (`src/app/`) — композиционные корни (Composition Root).
Сборка, прошивка и конфигурация — в [README скетча](../saspik-iot-sketches/esp32-boilerplate/README.md).
Контракт сообщений — единый JSON-конверт лога `{level,src,event,msg,topic,unitId,objectId,uptime[,cause]}` (см. [mqtt-topics](../../saspik-cluster/docs/mqtt-topics.md)).

### Общие решения

1. **DI через конструкторы/фабрики.** Модули зависят от интерфейсов (`IMqttClient`, `IEspNowClient`, `IConfigStore`, `ILogSink`…), реализации подставляются в App — это же даёт моки для host-тестов.
2. **Два документированных исключения из «без глобалов».** Переиспользуемые синглтоны `WifiConfig` и `DeviceLog` (shared-библиотеки, менять нельзя) изолированы за фасадами `CaptivePortal` и `IRebootCauseStore`; `static s_instance` внутри `MqttManager`/`EspNowManager` нужен для C-колбэков.
3. **Два источника конфигурации.**
   - Compile-time: `include/config.h` — пины, интервалы, unitId/objectId, словари полей, таймауты.
   - Runtime: NVS. WiFi/MQTT — через captive portal (namespace `saspik`); параметры приложения — через `ConfigManager` (namespace `saspik_app`), по умолчанию из config.h. Правило merge: defaults ← NVS (побеждает NVS).
4. **Формат на проводе.** MQTT — JSON (топики по [mqtt-topics](../../saspik-cluster/docs/mqtt-topics.md)). ESP-NOW — бинарный пакет v2 (см. README); JSON в эфире не гоняем (лимит 250 байт). Мост ESP-NOW↔MQTT делает перевод binary→JSON.
5. **Неблокирующий loop.** `yield()` в начале loop; MQTT connect не чаще `MQTT_RECONNECT_INTERVAL_MS` + socket timeout (защита от Task WDT, rst:0x8); авто-ребут по таймаутам WiFi/MQTT — только для Gateway/Device. Периодика — через `TimerRegistry` (overflow-safe разница `millis()`).
6. **Состав шаблонов.**
   - **Gateway**: CaptivePortal, ConfigManager, WifiManager, MqttManager, Logger (UART+MQTT), EspNowManager, EspNowMqttBridge, TimerRegistry; опционально (Gateway-как-Device) — свои ISensor/IActuator.
   - **Sensor**: CaptivePortal (только при первом запуске), Logger (UART + ESP-NOW-лог через шлюз), EspNowManager (tx), InterfaceManager + ISensor, LightSleepManager, TimerRegistry. MQTT/WiFi STA после настройки не используются.
   - **Device**: CaptivePortal, ConfigManager, Logger (UART + ESP-NOW), EspNowManager (rx/tx), InterfaceManager + ISensor/IActuator, TimerRegistry.

### Структура проекта

```
esp32-boilerplate/
├── platformio.ini              # env: {gateway,sensor,device}_{esp32,s2,s3,c3} + native
├── partitions.csv              # 4MB, раздел "spiffs" (LittleFS — для будущего OTA)
├── include/
│   └── config.h                # все compile-time константы (виден и src, и lib)
├── src/
│   ├── main.cpp                # выбор IApp по DEVICE_ROLE, yield(), loop
│   └── app/
│       ├── IApp.h              # контракт шаблона: begin()/update()
│       ├── GatewayApp.*        # Composition Root шлюза
│       ├── SensorApp.*         # Composition Root сенсора
│       ├── DeviceApp.*         # Composition Root устройства
│       └── examples/           # Dht22Sensor, FloatSensor, RelayActuator (не в библиотеке)
├── lib/saspik-core/            # переиспользуемая SOLID-библиотека
│   ├── include/saspik/         #   core, config, log, net, wifi, portal, mqtt,
│   │                           #   espnow, bridge, sys, interfaces
│   └── src/                    #   реализации (firmware-only — под #ifdef ARDUINO)
└── test/
    ├── mocks/                  # MockMqttClient, MockEspNowClient, MockWifiManager,
    │                           #   MockConfigStore, MockLogSink
    ├── test_topic_builder/     # host-тесты (Unity), по одной папке test_* на сьют
    ├── test_esp_now_codec/
    ├── test_bridge/
    ├── test_config_manager/
    └── test_timer_registry/
```

Замечания:
- `config.h` лежит в `include/` — PlatformIO добавляет его в include-путь и для `src/`, и для `lib/`. Роль-зависимые секции — через `#if DEVICE_ROLE == ...`.
- Публичные заголовки библиотеки — `lib/saspik-core/include/saspik/...`; приватные cpp — рядом в `src/`.
- shared-библиотеки (`wifi-config`, `device-log`) не копируются — подключаются через `lib_extra_dirs = ../shared`.

### Интерфейсы и классы

Зависимости идут только вниз; App не переиспользуется никем.

##### Интерфейсы

| Интерфейс | Ответственность |
| :--- | :--- |
| `IComponent` | Жизненный цикл: `begin()` / `update()` — общий контракт всех менеджеров |
| `IApp` | Контракт шаблона приложения: `begin()`, `update()`. Каждый App — Composition Root |
| `ILogSink` | Приём готовой записи лога для вывода; не знает о JSON/топиках |
| `IRebootCauseStore` | Чтение/запись причины ребута (фасад над shared `DeviceLog`) |
| `IConfigStore` | NVS «ключ-значение» (обёртка Preferences); позволяет мокать merge-логику |
| `ICaptivePortal` | Фасад над shared `WifiConfig`: begin/handlePortal/isPortalMode |
| `IWifiManager` | Жизненный цикл STA: reconnect по интервалу, авто-ребут при потере WiFi |
| `IMqttClient` | connect/publish/subscribe/isConnected/loop + регистрация `IMessageHandler` |
| `IMessageHandler` | Обработка одного входящего MQTT-сообщения (команды устройства, мост) |
| `IEspNowClient` | init радио, addPeer, send, onReceive-колбэк |
| `ISensor` | `begin()`, `read(SensorSample&)`, `getName()` — пары fieldId+value |
| `IActuator` | `begin()`, `setState(...)`, `getState()` — управляемый выход |
| `ISleepManager` | Подготовка и вход в light sleep; запрет сна в режиме портала |
| `IBus` | Инициализация шины; база для I2cBus/OneWireBus/SpiBus/UartBus |

##### Классы-менеджеры

| Класс | Ответственность (почему существует) |
| :--- | :--- |
| `ConfigManager` | Единственный владелец RuntimeConfig; собирает из defaults(config.h) ← NVS |
| `Logger` | Формирует JSON-конверт лога, рассылает по `ILogSink`; startup-конверт с причиной ребута |
| `MqttLogSink` | Публикация конверта в `device/.../log`; при offline — кольцевая очередь (drop-oldest) |
| `UartLogSink` | Вывод лога в Serial |
| `WifiManager` | STA: восстановление с интервалом, авто-ребут по `WIFI_REBOOT_TIMEOUT_MS` |
| `CaptivePortal` | Тонкий фасад над shared `WifiConfigManager`; скрывает глобальный синглтон |
| `MqttManager` | Адаптер PubSubClient: clientId от MAC, LWT offline + retained online, non-blocking connect, resubscribe, авто-ребут; диспетчеризация входящих |
| `EspNowManager` | Радио ESP-NOW: init (канал STA), реестр пиров, send; RX-буфер однослотовый, доставка в update() |
| `EspNowCodec` | Чистая сериализация пакета (host-тестируем без железа) |
| `EspNowMqttBridge` | Мост (Gateway): DATA→JSON→`sensor/...`, LOG→`.../log`, MQTT-команда→ESP-NOW |
| `TopicBuilder` | Построение всех топиков по unitId/objectId/суффиксу — единственное место схемы |
| `InterfaceManager` | Фабрика шин по конфигу; раздаёт шины сенсорам/исполнителям (инверсия контроля) |
| `LightSleepManager` | Таймерное пробуждение, накопленное время в RTC, вход в сон после flush лога |
| `TimerRegistry` | Неблокирующая периодика, overflow-safe проверка `millis()` |
| `GatewayApp` / `SensorApp` / `DeviceApp` | Композиционные корни: только wiring графа объектов, без бизнес-логики |

### Зависимости и слои

```
main.cpp  (setup/loop: yield, выбор шаблона, portal.handlePortal)
   │ DEVICE_ROLE
   ▼
IApp  ◄── GatewayApp / SensorApp / DeviceApp          [Composition Root]
   │
   ├──► ICaptivePortal ──► WifiConfigManager   (shared, синглтон, КАК ЕСТЬ)
   ├──► ConfigManager ──► NvsConfigStore (saspik_app)
   ├──► WifiManager ──► Arduino WiFi (STA)
   ├──► Logger ──► UartLogSink / MqttLogSink / EspNowLogSink
   │        └──► RebootCauseStore ──► DeviceLog   (shared, синглтон, КАК ЕСТЬ)
   ├──► MqttManager ──► PubSubClient ──► WiFiClient   (Gateway/Device)
   ├──► EspNowManager ──► esp_now
   ├──► EspNowMqttBridge   (только Gateway)
   ├──► InterfaceManager ──► IBus ──► шины ──► ISensor / IActuator
   ├──► LightSleepManager   (только Sensor)
   └──► TimerRegistry
```

Слои (проверка: нет циклов):

```
APP     : main.cpp, IApp, GatewayApp/SensorApp/DeviceApp
DOMAIN  : Logger, EspNowMqttBridge, EspNowCodec, TopicBuilder, TimerRegistry, RuntimeConfig
MANAGERS: ConfigManager, WifiManager, MqttManager, EspNowManager, InterfaceManager,
          CaptivePortal, LightSleepManager
INFRA   : PubSubClient/WiFiClient, esp_now, Arduino WiFi, Preferences, Serial
REUSED  : WifiConfigManager, DeviceLogManager (shared)
```

Правила:
- App знает всех; менеджеры знают только переданные в конструктор интерфейсы.
- Мост не знает GatewayApp; Logger не знает MqttManager (только `IMqttClient` через sink).
- `config.h` видят все слои, но менять значения разрешено только в App через defaults.

### Модули

**CaptivePortal.** Без изменений кода shared-библиотеки: `begin()` блокирует до 20 с на STA либо поднимает AP (DNS+HTTP), `/save` пишет в NVS и делает `ESP.restart()`. «Нет NVS → сразу портал» покрывает первый запуск.

**ConfigManager.** Merge-правило: defaults (config.h) ← NVS (saspik_app) ← DeviceConfig из портала (saspik, последний побеждает). config.h — единственное место «заводских» значений.

**Logger.** Собирает конверт и раздаёт по sink'ам. Startup-конверт с причиной ребута — после первого успешного MQTT-подключения. UART — всегда; MQTT/ESP-NOW — когда транспорт жив, иначе очередь drop-oldest.

**MqttManager.** Неблокирующий connect (интервал + socket timeout — против rst:0x8), retained online + LWT offline в `/status`, resubscribe после reconnect, авто-ребут по `MQTT_REBOOT_TIMEOUT_MS`, колбэк PubSubClient через статический указатель на this.

**EspNowManager + EspNowCodec.** Кодек: `[type(1)][timestamp LE(4)][fieldId(1)+len(1)+data]*[crc8(1)]`, CRC-8/MAXIM; типы DATA/LOG/COMMAND/ACK; служебные поля `0xFF`/`0xFE` — JSON лога/команды. Несовместим с v1 (node-sensor-1) — миграция узлов отдельная задача. Менеджер принимает пакеты в однослотовый RX-буфер и отдаёт их в update(), не в колбэке.

**EspNowMqttBridge.** Единственное место перевода binary↔JSON: узел→MQTT (data→JSON по fieldDict, log→конверт), MQTT→узел (команда→ESP-NOW как JSON без разбора). Маппинг objectId→MAC и словарь полей — статическая таблица в config.h.

**InterfaceManager + шины.** Фабрика создаёт по конфигу только нужные шины (пины — compile-time) и отдаёт их датчикам через конструкторы — инверсия контроля упрощает моки и переиспользование.

**LightSleepManager.** Таймерное пробуждение (`esp_sleep_enable_timer_wakeup`), накопленное время в RTC-памяти, отложенный вход в сон после flush лога, запрет сна при активном портале.

**TimerRegistry.** Задачи: чтение сенсоров, отправка данных, проверка сна. `yield()` вызывается в loop всегда, вне registry.

### Открытые вопросы

1. **Light sleep + ESP-NOW (критично для Sensor).** Штатный light sleep гасит радио — команды не доходят. Варианты: one-way TX (надёжно, но нет приёма), radio-on light sleep (приём возможен, но жрёт энергию — нужен стенд и замеры), store-and-forward на шлюзе. Стратегия канала ESP-NOW при роуминге STA — подтвердить.
2. **Runtime-конфиг вне портала.** Портальная форма — только WiFi/MQTT. Интервалы, peer MAC, словари полей — defaults в config.h + переопределение через MQTT-команды в v2; без изменения shared `WifiConfig`.
3. **Адресация команд узлам.** Проверить ACL брокера для `device/{unitId}/+`; объём и формат маппинга nodeObjectId→MAC и fieldDict на шлюзе (config.h v1 / NVS v2).
