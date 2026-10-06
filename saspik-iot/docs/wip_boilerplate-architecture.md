# Архитектура boilerplate прошивок ESP32 (SOLID) — saspik-core

**Статус:** проектное решение для последующей генерации кода (ПРОМПТ 2).
**Основа:**
- `saspik-iot/saspik-iot-sketches/esp32-local-mqtt` — WifiConfig (переиспользуем КАК ЕСТЬ), DeviceLog, неблокирующий MQTT/WiFi reconnect, авто-ребут, JSON-конверт лога.
- `saspik-iot/saspik-iot-sketches/shared/wifi-config` — captive portal (WifiConfigManager, синглтон).
- `saspik-iot/saspik-iot-sketches/shared/device-log` — причина ребута в NVS (DeviceLogManager, синглтон).
- `saspik-iot/saspik-iot-sketches/saspik-iot-node-sensor-1` — бинарный ESP-NOW транспорт, ISensor, SleepManager (deep sleep).
**Контракт сообщений:** `saspik-cluster/docs/mqtt-topics.md`, `docs.html` (единый конверт лога `{level,src,event,msg,topic,unitId,objectId,uptime[,cause]}`).

---

## 0. Общие решения

1. **Один PlatformIO-проект, три шаблона.** Шаблон выбирается build-флагом `DEVICE_ROLE=GATEWAY|SENSOR|DEVICE` (отдельные env в platformio.ini). Переиспользуемая логика — библиотека `lib/saspik-core`. Приложения-шаблоны (`src/app/`) — композиционные корни (Composition Root): только они знают, как собрать граф объектов.
2. **DI через конструкторы/фабрики.** Модули зависят от интерфейсов (`IMqttClient`, `IEspNowClient`, `IConfigStore`, `ILogSink`...), реализации подставляются в App. Это же даёт возможность моков в host-тестах.
3. **Единственное исключение из "без глобалов".** Переиспользуемые `WifiConfigManager WifiConfig` и `DeviceLogManager DeviceLog` — синглтоны (extern в их заголовках, менять нельзя — "КАК ЕСТЬ"). Они изолированы за фасадами `CaptivePortal` и `IRebootCauseStore`; весь остальной код про них не знает.
4. **Два источника конфигурации.**
   - Compile-time: `include/config.h` (роль, unitId/objectId, пины, интервалы, peer MAC, словарь полей fieldId→key, таймауты).
   - Runtime: NVS. WiFi/MQTT — через существующий WifiConfigManager (namespace `saspik`). Параметры приложения (интервал сна/чтения, топики, peers) — отдельный namespace `saspik_app` через `IConfigStore`; по умолчанию берутся из config.h. (Расширение портала запрещено — см. Вопрос 3.)
5. **Формат на проводе:**
   - MQTT — JSON по mqtt-topics.md: данные `sensor/{unitId}/{objectId}`, команды `device/{unitId}/{objectId}`, лог `device/{unitId}/{objectId}/log`, статус `device/{unitId}/{objectId}/status` (retained online + LWT offline).
   - ESP-NOW — бинарный пакет (наследие EspNowTransport): `[type(1)][ts(4)][fieldId(1)+len(1)+data]*[crc8(1)]`. JSON в эфире не гоняем (лимит 250 байт, уже проверено в node-sensor-1). Мост ESP-NOW↔MQTT делает перевод binary→JSON (см. Вопрос 2).
6. **Неблокирующий loop.** Наследуем уроки esp32-local-mqtt: `yield()` в начале loop; MQTT connect не чаще `MQTT_RECONNECT_INTERVAL_MS` + socket timeout, чтобы не ловить Task WDT (rst:0x8); авто-ребут по таймаутам WiFi/MQTT — только для Gateway/Device (Sensor спит — политика отдельно, см. Вопрос 1). Периодика — через `TimerRegistry` (overflow-safe разница `millis()`).
7. **Состав шаблонов.**
   - Gateway: CaptivePortal, ConfigManager, WifiManager, MqttManager, Logger (UART+MQTT), EspNowManager, EspNowMqttBridge, TimerRegistry; опционально (флаг `HAS_OWN_PERIPHERALS`) — InterfaceManager + ISensor/IActuator для собственных данных (Gateway-как-Device, см. Вопрос 5).
   - Sensor: CaptivePortal (только при первом запуске), ConfigManager, Logger (UART + ESP-NOW-лог через шлюз), EspNowManager, InterfaceManager + ISensor, LightSleepManager, TimerRegistry. MQTT/WiFi STA после настройки не используются (ESP-NOW работает без ассоциации с AP) — см. Вопрос 1.
   - Device: CaptivePortal, ConfigManager, Logger, EspNowManager (rx/tx), InterfaceManager + ISensor/IActuator, TimerRegistry.

---

## 1. Дерево файлов

```
saspik-iot/saspik-iot-sketches/esp32-boilerplate/
├── platformio.ini                     # env: gateway / sensor / device / test-native;
│                                      #   lib_extra_dirs = ../shared (wifi-config, device-log)
├── partitions.csv                     # копия из esp32-local-mqtt: 4MB, раздел назван "spiffs"
├── .clang-format                      # Google, 4 пробела, ширина 100
├── LICENSE                            # MIT + SPDX
├── README.md                          # инструкция сборки и прошивки (заполняется в ПРОМПТ 2)
├── TODO.md                            # открытые вопросы и заглушки
│
├── include/
│   └── config.h                       # compile-time константы (виден и src, и lib через include-путь)
│
├── src/
│   ├── main.cpp                       # setup/loop: выбор IApp по DEVICE_ROLE, yield(), portal.handlePortal()
│   └── app/
│       ├── IApp.h                     # begin()/update() — контракт шаблона
│       ├── GatewayApp.h / GatewayApp.cpp
│       ├── SensorApp.h  / SensorApp.cpp
│       └── DeviceApp.h  / DeviceApp.cpp
│
├── lib/
│   └── saspik-core/
│       ├── library.json
│       ├── include/saspik/
│       │   ├── core/
│       │   │   ├── IComponent.h           # begin()/update() — базовый жизненный цикл
│       │   │   ├── DeviceInfo.h           # unitId/objectId/роль (value type)
│       │   │   ├── TimerRegistry.h        # неблокирующая периодика
│       │   │   ├── config/RuntimeConfig.h # DeviceConfig(wifi/mqtt) + AppConfig(интервалы, peers, поля)
│       │   │   ├── config/IConfigStore.h  # абстракция NVS (Preferences)
│       │   │   ├── config/ConfigManager.h # merge defaults(config.h) + NVS + portal
│       │   │   ├── log/ILogSink.h
│       │   │   ├── log/Logger.h           # JSON-конверт, роутинг по ILogSink
│       │   │   ├── log/UartLogSink.h
│       │   │   ├── log/MqttLogSink.h      # публикация в device/.../log, очередь при offline
│       │   │   ├── log/IRebootCauseStore.h# фасад над DeviceLogManager (shared)
│       │   │   ├── net/TopicBuilder.h     # построение всех топиков по unitId/objectId
│       │   │   ├── net/IMessageHandler.h  # обработчик входящих MQTT-команд
│       │   │   ├── wifi/IWifiManager.h
│       │   │   ├── wifi/WifiManager.h     # STA connect/reconnect/reboot-watchdog
│       │   │   ├── portal/ICaptivePortal.h
│       │   │   ├── portal/CaptivePortal.h # фасад над WifiConfigManager (shared, КАК ЕСТЬ)
│       │   │   ├── mqtt/IMqttClient.h
│       │   │   ├── mqtt/MqttManager.h     # PubSubClient-адаптер, LWT, sub/pub, watchdog
│       │   │   ├── espnow/IEspNowClient.h
│       │   │   ├── espnow/EspNowManager.h # radio + peers + rx/tx
│       │   │   ├── espnow/EspNowCodec.h   # [type][ts][fieldId+len+data]*[crc8]
│       │   │   ├── bridge/EspNowMqttBridge.h # мост ESP-NOW↔MQTT (только Gateway)
│       │   │   └── sys/ISleepManager.h
│       │   │   └── sys/LightSleepManager.h# light sleep по таймеру, RTC-millis (только Sensor)
│       │   └── interfaces/
│       │       ├── bus/IBus.h             # begin()
│       │       ├── bus/I2cBus.h OneWireBus.h SpiBus.h UartBus.h
│       │       ├── bus/InterfaceManager.h # фабрика шин по конфигу
│       │       ├── sensor/ISensor.h       # begin()/read(SensorSample&)/getName()
│       │       ├── sensor/SensorSample.h  # fieldId+value (feed для кодера ESP-NOW/JSON)
│       │       └── actuator/IActuator.h   # begin()/setState()/getState()
│       └── src/
│           ├── core/TimerRegistry.cpp  DeviceInfo.cpp
│           ├── config/ConfigManager.cpp
│           ├── log/Logger.cpp UartLogSink.cpp MqttLogSink.cpp RebootCauseStore.cpp
│           ├── net/TopicBuilder.cpp
│           ├── wifi/WifiManager.cpp
│           ├── portal/CaptivePortal.cpp
│           ├── mqtt/MqttManager.cpp
│           ├── espnow/EspNowManager.cpp EspNowCodec.cpp
│           ├── bridge/EspNowMqttBridge.cpp
│           ├── sys/LightSleepManager.cpp
│           └── interfaces/I2cBus.cpp OneWireBus.cpp SpiBus.cpp UartBus.cpp InterfaceManager.cpp
│
├── test/
│   ├── native/                          # PlatformIO env:native (host-тесты, фреймворк — см. Вопрос 7)
│   │   ├── mocks/
│   │   │   ├── MockMqttClient.h  MockEspNowClient.h
│   │   │   ├── MockWifiManager.h MockConfigStore.h
│   │   │   └── MockLogSink.h
│   │   ├── test_topic_builder.cpp       # чистая логика, без железа
│   │   ├── test_esp_now_codec.cpp
│   │   ├── test_bridge.cpp              # mapping ESP-NOW↔MQTT
│   │   ├── test_config_manager.cpp      # merge defaults/NVS/portal
│   │   └── test_timer_registry.cpp
│   └── TODO.md                          # smoke-тесты на железе (радио/MQTT) — вынести позже
│
└── (shared-библиотеки не копируются, подключаются через lib_extra_dirs = ../shared)
```

Замечания:
- `config.h` лежит в `include/` — PlatformIO добавляет его в include-путь и для `src/`, и для `lib/`, поэтому все модули видят compile-time константы. Роль-зависимые секции — через `#if DEVICE_ROLE == ...`.
- Публичные заголовки библиотеки — `lib/saspik-core/include/saspik/...`; приватные cpp — рядом в `src/`.
- Примеры датчиков/исполнителей (`Dht22Sensor`, `FloatSensor`, `RelayActuator`) НЕ входят в библиотеку — это примеры в `src/app/` (показывают, как пользоваться ISensor/IActuator).

---

## 2. Интерфейсы и классы: ответственности (SRP)

### Интерфейсы (зависимости "вверх", цель — тестируемость и DI)

| Интерфейс | Ответственность |
| :--- | :--- |
| `IComponent` | Жизненный цикл: `begin()` (инициализация, true при успехе), `update()` (неблокирующий тик). Общий контракт всех менеджеров. |
| `ILogSink` | Приём готовой записи лога (`LogRecord`) для вывода. Не знает о JSON/топиках. |
| `IRebootCauseStore` | Чтение/запись причины последнего ребута (фасад над shared/device-log). Извлекает синглтон `DeviceLog` за интерфейс. |
| `IConfigStore` | Доступ к NVS «ключ-значение» (обёртка Preferences). Позволяет мокать и тестировать merge-логику ConfigManager на хосте. |
| `ICaptivePortal` | Фасад: `begin(DeviceConfig&, buttonPin, defaults)` → true если STA подключён, false если портал; `handlePortal()`; `isPortalMode()`. Скрывает синглтон `WifiConfig` и детали поведения (save → ESP.restart()). |
| `IWifiManager` | Жизненный цикл STA: статус, reconnect по интервалу, авто-ребут при длительной потере WiFi. |
| `IMqttClient` | connect (неблокирующий), publish, subscribe, isConnected, loop, регистрация `IMessageHandler` по топику. Абстракция над PubSubClient для моков. |
| `IMessageHandler` | Обработка одного входящего MQTT-сообщения (topic + payload). Реализуют: команды устройства, мост (команды для ESP-NOW-узлов). |
| `IEspNowClient` | init радио, addPeer (MAC), send (пакет), onReceive-колбэк. Абстракция над esp_now для моков. |
| `IBus` | Инициализация шины (`begin()`). База для четырёх шин. |
| `ISensor` | `begin()`, `read(SensorSample&)`, `getName()`. Датчик возвращает пары fieldId+value — источник данных для кодера и JSON. |
| `IActuator` | `begin()`, `setState(...)`, `getState()`. Управляемый выход (реле, PWM...). |
| `ISleepManager` | Подготовка и вход в light sleep; запрет сна в режиме портала; учёт накопленного времени (RTC). |
| `IApp` | Контракт шаблона приложения: `begin()`, `update()`. Каждый App — Composition Root. |

### Классы-менеджеры (по одному делу на класс)

| Класс | Ответственность (почему существует) |
| :--- | :--- |
| `ConfigManager` | Единственный владелец актуального `RuntimeConfig`. Собирает его из трёх источников: defaults (config.h) → NVS (saspik_app) → DeviceConfig (saspik, из портала). Раздаёт по const-ссылке. |
| `RuntimeConfig` / `DeviceInfo` | Value types: `DeviceConfig` (из wifi-config, поля WiFi/MQTT) + `AppConfig` (sleepIntervalMs, sensorIntervalMs, espNowChannel, peers[], fieldDict). Никакой логики. |
| `Logger` | Формирует единый JSON-конверт лога (level/src="device"/event/msg/topic/unitId/objectId/uptime/cause), дописывает uptime и identity, рассылает по `ILogSink`. Отправляет startup-конверт с причиной ребута (`IRebootCauseStore`). Не знает, кто потребители. |
| `UartLogSink` | Вывод в Serial (print). |
| `MqttLogSink` | Публикация конверта в `device/.../log` через `IMqttClient`. При offline — ограниченная очередь (drop старых, счётчик потерь). Не знает формат конверта — только топик и строку. |
| `WifiManager` | Только STA: статус, восстановление с интервалом, авто-ребут при `WIFI_REBOOT_TIMEOUT_MS` (перенос логики `handleWifiReconnect` из esp32-local-mqtt). |
| `CaptivePortal` | Тонкий фасад над WifiConfigManager: делегирование begin/handlePortal/isPortalMode; скрытие глобального синглтона. Сама логика портала — в shared-библиотеке, не дублируется. |
| `MqttManager` | Адаптер PubSubClient: clientId от MAC, LWT offline + retained online в `/status`, non-blocking connect (интервал + socket timeout — против rst:0x8), подписки, диспетчеризация входящих по топику в `IMessageHandler`, авто-ребут при `MQTT_REBOOT_TIMEOUT_MS`. |
| `EspNowManager` | Радио ESP-NOW: init (после WifiManager, один и тот же канал), реестр пиров из `RuntimeConfig`, отправка `EspNowPacket`, приём → колбэк. Сам формат не знает — отдаёт/принимает байты. |
| `EspNowCodec` | Чистая сериализация/десериализация пакета: `[type(1)][ts(4)][fieldId(1)+len(1)+data]*[crc8(1)]`. Типы: data/log/command/ack. Host-тестируем без железа. |
| `EspNowMqttBridge` | Мост (только Gateway): ESP-NOW data → JSON → `sensor/{unitId}/{nodeObjectId}`; ESP-NOW log → конверт → `device/{unitId}/{nodeObjectId}/log`; MQTT-команда → ESP-NOW по маппингу objectId→MAC. Полевая dictionary fieldId→key. |
| `TopicBuilder` | Построение всех топиков по unitId/objectId/суффиксу. Единственное место, где живёт схема `{домен}/{unitId}/{objectId}[/суффикс]`. |
| `InterfaceManager` | Фабрика и владелец шин: создаёт `I2cBus`/`OneWireBus`/`SpiBus`/`UartBus` по конфигу (пины — compile-time), раздаёт ссылки на шины сенсорам/исполнителям. |
| `I2cBus`, `OneWireBus`, `SpiBus`, `UartBus` | Обёртки над Wire/OneWire/SPI/HardwareSerial: инициализация пинов/частоты, доступ к хэндлу шины. |
| `LightSleepManager` | Таймерное пробуждение (`esp_sleep_enable_timer_wakeup`), накопленное время в RTC-памяти, отложенный вход в сон после flush лога, запрет сна при активном портале. |
| `TimerRegistry` | Периодические задачи (callback+интервал), overflow-safe проверка `millis()`. Единственный источник «тикать сейчас?». |
| `GatewayApp` / `SensorApp` / `DeviceApp` | Композиционные корни: строят граф через конструкторы, задают порядок begin, связывают колбэки (esp-now→bridge, mqtt→handlers, таймеры→read). Содержат только wiring, без бизнес-логики. |

### Классы-примеры (в `src/app/`, не в библиотеке)

| Класс | Ответственность |
| :--- | :--- |
| `Dht22Sensor`, `FloatSensor` | Примеры ISensor (перенос из node-sensor-1, через шину/пин из конфига). |
| `RelayActuator` | Пример IActuator для Gateway-как-Device. |

---

## 3. Диаграмма зависимостей (текст)

Направление стрелки — «зависит от» / «использует». Зависимости идут только вниз; App не переиспользуется никем.

```
main.cpp  (setup/loop: yield, выбор шаблона, portal.handlePortal)
   │ DEVICE_ROLE
   ▼
IApp  ◄── GatewayApp / SensorApp / DeviceApp          [Composition Root, строят граф]
   │
   ├──► ICaptivePortal ──► CaptivePortal ──► WifiConfigManager   (shared, синглтон, КАК ЕСТЬ)
   ├──► ConfigManager ──► IConfigStore ──► Preferences (NVS saspik_app)
   │        └─ RuntimeConfig = DeviceConfig + AppConfig (defaults из config.h)
   ├──► IWifiManager ──► WifiManager ──► Arduino WiFi (STA)
   ├──► Logger ──► ILogSink ──► UartLogSink ──► Serial
   │        └──────► MqttLogSink ──► IMqttClient          (только Gateway/Device)
   │        └──────► IRebootCauseStore ──► DeviceLogManager (shared, синглтон)
   ├──► IMqttClient ──► MqttManager ──► PubSubClient ──► WiFiClient
   │        └─ TopicBuilder; LWT/status; dispatch ► IMessageHandler
   ├──► IEspNowClient ──► EspNowManager ──► esp_now
   │        └─ EspNowCodec  (binary: [type][ts][fieldId+len+data]*[crc8])
   ├──► EspNowMqttBridge  (только Gateway)
   │        ├─► IEspNowClient  (rx: data/log от узлов)
   │        ├─► IMqttClient    (tx: sensor/.../log; rx: команды)
   │        └─► TopicBuilder + fieldDict (fieldId → key)
   ├──► InterfaceManager ──► IBus ──► I2cBus / OneWireBus / SpiBus / UartBus
   │        └─► ISensor (Dht22Sensor...) / IActuator (RelayActuator...)
   ├──► ISleepManager ──► LightSleepManager ──► esp_sleep   (только Sensor)
   └──► TimerRegistry  (периодика: read сенсоров, sleep check, diag)
```

Слои (проверка: нет циклов):

```
APP     : main.cpp, IApp, GatewayApp/SensorApp/DeviceApp
DOMAIN  : Logger, EspNowMqttBridge, EspNowCodec, TopicBuilder, TimerRegistry, RuntimeConfig
MANAGERS: ConfigManager, WifiManager, MqttManager, EspNowManager, InterfaceManager,
          CaptivePortal, LightSleepManager
INFRA   : PubSubClient/WiFiClient, esp_now, Arduino WiFi, Preferences, Serial,
          Wire/OneWire/SPI/HardwareSerial
REUSED  : WifiConfigManager (shared/wifi-config), DeviceLogManager (shared/device-log)
```

Правила:
- App знает всех; менеджеры знают только интерфейсы/соседние менеджеры, переданные в конструктор.
- Мост не знает GatewayApp; Logger не знает MqttManager (только IMqttClient через MqttLogSink).
- `config.h` видят все слои (include-путь), но менять его значения разрешено только в App через defaults (ConfigManager).

---

## 4. Описание модулей

**saspik-core (lib/).** Набор независимых модулей с единственной ответственностью. Порядок begin в App: CaptivePortal → ConfigManager → Logger → WifiManager → MqttManager (Gateway/Device) → EspNowManager → EspNowMqttBridge → InterfaceManager+sensors/actuators → LightSleepManager → регистрация задач в TimerRegistry. Порядок важен: ESP-NOW требует поднятого WiFi-радио (одинаковый канал с сетью/шлюзом).

**CaptivePortal + WifiConfigManager.** Без изменений кода библиотеки: `begin()` блокирует до 20 с на STA-подключение либо поднимает AP (DNS+HTTP), `/save` сохраняет в NVS и делает `ESP.restart()`. Фасад транслирует это в `ICaptivePortal`. В `loop()` портал обслуживается до выхода из `isPortalMode()`. После рестарта устройство стартует в штатном режиме. Поведение «нет NVS → сразу портал» покрывает первый запуск сенсора/девайса без прошивки конфига.

**ConfigManager.** Merge-правило: defaults (config.h) ← NVS overrides (saspik_app) ← DeviceConfig из портала (saspik). Последний источник побеждает. При отсутствии ключа в NVS берётся default — поэтому config.h остаётся единственным местом «заводских» значений, а NVS — единственным местом runtime-переопределений.

**Logger (UART + MQTT).** Собирает конверт `{level,src:"device",event,msg,topic,unitId,objectId,uptime[,cause]}` (совместимо с telegraf/Influx из кластера) и раздаёт по sink'ам. Startup-конверт с причиной ребута — через `IRebootCauseStore` после первого успешного подключения MQTT (поведение `publishStartupLog` из esp32-local-mqtt). UART-вывод — всегда; MQTT — когда соединение живо, иначе буфер с ограничением (политика — Вопрос 4).

**WifiManager.** Перенос `handleWifiReconnect`: восстановление с интервалом `WIFI_RECONNECT_INTERVAL_MS`, авто-ребут при недоступности WiFi дольше `WIFI_REBOOT_TIMEOUT_MS`. Для Sensor политика другая: STA нужен только при первом конфигурировании и синхронизации канала; в рабочем цикле — ESP-NOW без ассоциации с AP, reboot-watchdog отключён (устройство спит).

**MqttManager.** Адаптер PubSubClient: clientId от MAC (`{role}-{mac без ':'}`), non-blocking connect (интервал + `setSocketTimeout`/`setTimeout` — защита от Task WDT), retained `{"status":"online"}` + LWT offline, авто-ребут при `MQTT_REBOOT_TIMEOUT_MS`, диспетчеризация входящих по топику в зарегистрированные `IMessageHandler`. Callback PubSubClient — свободная функция; внутри переадресация на экземпляр (принятый паттерн, статический указатель на this).

**EspNowManager + EspNowCodec.** Менеджер: init радио (канал из конфига, должен совпадать с каналом STA/AP шлюза), реестр пиров (MAC из RuntimeConfig), send/recv. Кодек: бинарный пакет `[type][ts][fieldId+len+data]*[crc8]`, типы data/log/command/ack — расширение протокола node-sensor-1 для поддержки логов и команд (нужно мосту и двусторонней связи). Кодек не зависит от железа — покрывается host-тестами.

**EspNowMqttBridge.** Направление узел→MQTT: пакет data → JSON по fieldDict → `sensor/{unitId}/{nodeObjectId}`; пакет log → конверт лога → `device/{unitId}/{nodeObjectId}/log`. Направление MQTT→узел: подписка на командные топики узлов → encode → send на MAC из маппинга objectId→MAC; ack-пакеты от узла при необходимости (TODO). Это единственное место, где живёт перевод binary↔JSON.

**InterfaceManager + шины.** Фабрика создаёт по конфигу шины I2C/OneWire/SPI/UART (пины/частоты — compile-time) и отдаёт их датчикам/исполнителям через конструкторы. Датчики не инициализируют шины сами (инверсия контроля) — это упрощает моки и переиспользование одного и того же датчика на разных шинах.

**LightSleepManager.** Таймерное пробуждение из light sleep с интервалом из RuntimeConfig (`sleepIntervalMs`). Накопленное время — в RTC-памяти (наследие SleepManager из node-sensor-1). Вход в сон: только после flush логов и при неактивном портале. Пробуждение по GPIO (аварийные события) — TODO.

**TimerRegistry.** Неблокирующая периодика с overflow-safe разницей времени. Задачи: чтение сенсоров, отправка данных, диагностика, проверка сна, WDT-feeding (yield вызывается в loop всегда, вне registry).

**Шаблоны приложений (src/app/).** GatewayApp собирает полный граф (включая мост и опционально свои датчики/реле), SensorApp — минимальный (порт, лог, ESP-NOW, сон), DeviceApp — без сна, с rx/tx и исполнителями. Каждый App реализует `IApp`; `main.cpp` не содержит логики — только выбор и два вызова.

---

## 5. Открытые вопросы (TODO для следующего шага)

1. **Light sleep + ESP-NOW (критично для Sensor).**
   - Приём во сне: штатный light sleep гасит радио — входящие ESP-NOW не доходят. Варианты: (а) Sensor — one-way TX: проснулся по таймеру → отправил → сон (надёжно, но нет приёма команд/настроек); (б) radio-on light sleep (`esp_wifi_set_sleep_type(WIFI_PS_NONE)` + light sleep) — приём возможен, но энергопотребление заметно выше, поведение разное на ESP32/S2/S3/C3 — нужен стенд и замеры; (в) store-and-forward: шлюз держит сообщения, сенсор забирает при пробуждении (нужен ack + очередь на шлюзе). **TODO: эксперимент на реальном железе, до выбора варианта SleepManager не финализировать.**
   - Канал: ESP-NOW требует фиксированного канала; при роуминге STA канал меняется. Для сенсора: фиксированный канал из конфига (или краткое STA-подключение для синхронизации при старте). **TODO: подтвердить стратегию канала.**
   - Конфликт с авто-ребутами (WiFi/MQTT timeout): на Sensor reboot-watchdog не применяется. Подтвердить политику.
   - `WifiConfig.begin()` блокирует до 20 с на STA при каждом старте — для батарейного сенсора это дорого. Вариант: одноразовая настройка + флаг «configured», далее пропуск STA. Но «КАК ЕСТЬ» — нельзя менять. **TODO: решить (обёртка с таймаутом? fork? оставить 20 с?).**

2. **Формат ESP-NOW payload.** Бинарный `[type][ts][fieldId+len+data]*[crc8]` (наследие node-sensor-1, компактно, проверено) против JSON-в-эфире (лимит 250 байт, унификация формата). Предложение: binary в эфире + JSON на MQTT, кодек и словарь полей — общие. **Подтвердить, что отказ от JSON-в-эфире приемлем.**

3. **Runtime-конфиг вне портала.** Портальная форма содержит только WiFi/MQTT (6 полей). Интервалы сна/чтения, peer MAC, словарь полей через портал задать нельзя без изменения WifiConfig («КАК ЕСТЬ» нарушается). Предложение: defaults в config.h + переопределение через MQTT-команды (`device/{unitId}/{objectId}`) в v2; TODO на этот механизм. Альтернатива — форк портала (нарушение требования). **Нужно решение.**

4. **Политика очереди логов при offline (MqttLogSink).** Ограниченный буфер (drop-старых) или потеря с индикацией? Сколько записей (RAM budget)? В esp32-local-mqtt был кольцевой файловый лог на LittleFS — **убран** ради стабильности; не возвращаем без явной задачи.

5. **Gateway-как-Device (критично).**
   - Разделение трафика: собственные данные шлюза → `sensor/{unitId}/{gatewayObjectId}`, узлы → `sensor/{unitId}/{nodeObjectId}`. Гарантия уникальности objectId (классификация `saspik.{роли}.{протокол}.{питание}{nnn}`, номера 001..999).
   - Адресация команд узлам: подписка `device/{unitId}/+` (зависит от ACL брокера mosquitto — в `mqtt-topics.md` ACL для `device/+/+/log` есть, для `device/{unitId}/+` — нет) либо суффикс `device/{unitId}/{gatewayId}/espnow/{mac}`. **TODO: проверить ACL и согласовать схему с кластером.**
   - Маппинг nodeObjectId→MAC и fieldDict на шлюзе: статическая таблица в config.h (v1) / NVS (v2). **TODO: объём и формат таблицы.**
   - Подтверждение: Device-роль для шлюза (свои ISensor/IActuator) включается флагом `HAS_OWN_PERIPHERALS` — ок ли такой состав?

6. **Синглтоны против «без глобалов».** WifiConfig/DeviceLog — глобальные объекты в shared-библиотеках, менять нельзя. Мы изолируем их за фасадами, но глобальный статус остаётся (один процесс, одна копия). Подтвердить, что это приемлемо как документированное исключение.

7. **Тестовый фреймворк.** Unity vs Google Test для env:native (Arduino core на хосте не собирается — тестируются только чистые модули: TopicBuilder, EspNowCodec, Bridge-mapping, ConfigManager-merge, TimerRegistry; радио/MQTT — smoke-тесты на железе, отдельно). **Выбор за нами; ПРОМПТ 2 генерирует заготовки.**

8. **LittleFS в стеке.** В текущей схеме LittleFS не используется (лог сокращён до NVS). Оставить `partitions.csv`/`board_build.filesystem` для совместимости и будущего OTA/сертификатов, но в коде не задействовать, пока не появится задача.

9. **Прочие заглушки (из TODO.md, ПРОМПТ 2):** шифрование ESP-NOW (LMK/PMK), OTA, SNTP/time sync (uptime узлов vs реальное время — привет вопросу 1), TLS/secure boot. Пока интерфейсы-заглушки.

10. **Расположение проекта.** Предлагаю `saspik-iot/saspik-iot-sketches/esp32-boilerplate/` (сосед существующих скетчей, `lib_extra_dirs = ../shared` — как в esp32-local-mqtt). Подтвердить.