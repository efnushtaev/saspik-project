1. Бойлерплейт должен быть основой для разработки прошивок следующих шаблонов: шлюз, сенсор, девайс.
2. Бойлерплейт должен быть написан по принципам SOLID
3. Шлюз должен уметь 
	1. конфигрурироваться через captivePortal
	2. подключаться к wifi
	3. переподклбючаться после потери соединения
	4. формировать сообщения в лог
	5. подключаться к mqtt брокер
	6. подписываться к топикам
	7. писать сообщения в топики
	8. подключать esp-now
	9. получать и отправлять сообщения от подключенных esp-now устройств
4. Сенсор должен уметь
	1. конфигрурироваться через captivePortal
	2. формировать сообщения в лог
	3. подключать esp-now
	4. отправлять сообщения от подключенных esp-now устройств
	5. иметь низкое энергопотребление
	6. конфигурировать и использвать интерфейсы **I2C** **OneWire** spi или uart
5. Девайс должен уметь
	1. конфигрурироваться через captivePortal
	2. формировать сообщения в лог
	3.  подключать esp-now
	4. получать и отправлять сообщения от подключенных esp-now устройств
	5. конфигурировать и использвать интерфейсы **I2C** **OneWire** spi или uart
6. За основу нужно взять прошивку esp32-local-mqtt
7. Все переменные должны быть вынесены в конфигурационный файл
8. 

## ПРОМПТ 1

Роль: senior embedded C++ разработчик ESP32.

Задача: спроектировать архитектуру boilerplate для прошивок ESP32 (SOLID) на базе esp32-local-mqtt:
https://github.com/efnushtaev/saspik-project/tree/main/saspik-iot/saspik-iot-sketches/esp32-local-mqtt

Стек:
- C++17, Arduino framework, PlatformIO
- ESP32 / S2 / S3 / C3
- PubSubClient, LittleFS, NVS/Preferences
- Captive Portal — переиспользовать WifiConfig из esp32-local-mqtt КАК ЕСТЬ
- Логирование: UART + MQTT диагностика (как в esp32-local-mqtt)
- Формат сообщений — JSON (как в esp32-local-mqtt, см. docs.html)

Три шаблона:
- Gateway: captive portal, Wi-Fi + reconnect, MQTT connect/sub/pub, ESP-NOW, мост ESP-NOW↔MQTT, может быть и Device.
- Sensor: captive portal, лог, ESP-NOW, light sleep по таймеру (интервал в конфиге), I2C/OneWire/SPI/UART (абстракции).
- Device: captive portal, лог, ESP-NOW rx/tx, I2C/OneWire/SPI/UART (абстракции).

Архитектура:
- SOLID, DI через конструкторы/фабрики, без глобальных переменных.
- Общие модули: Logger, ConfigManager, WifiManager, CaptivePortal, MqttManager, EspNowManager, InterfaceManager (I2C/OneWire/SPI/UART).
- Compile-time константы — в config.h. Runtime — в NVS через captive portal.
- Неблокирующий loop (yield, WDT, таймеры).

Качество кода:
- Комментарии на русском, объясняют "почему", не "что".
- Doxygen для публичных методов, шапка файла с назначением.
- Naming: PascalCase классы, camelCase методы, m_ члены, UPPER_SNAKE константы.
- clang-format (Google, 4 пробела, 100), лицензия MIT + SPDX.
- Легко читаемый код: короткие функции, вложенность ≤3.

Выдай ТОЛЬКО:
1. Дерево файлов (lib/, src/, include/, test/).
2. Список интерфейсов и классов с ответственностями (SRP).
3. Диаграмму зависимостей текстом.
4. Описание модулей.
5. Открытые вопросы (особенно: light sleep + ESP-NOW, Gateway-как-Device).

Код не писать. Критичное не додумывать — помечать TODO.


## ПРОМПТ 2

Контекст: подтверждённая архитектура boilerplate ESP32 (SOLID) из предыдущего шага.
Стек: C++17, Arduino, PlatformIO, PubSubClient, LittleFS, NVS, esp32-local-mqtt (WifiConfig переиспользуем как есть).

Сгенерируй по одному файлу:
- все заголовочные и исходные файлы модулей
- config.h — все compile-time переменные
- platformio.ini — env для ESP32 / S2 / S3 / C3
- .clang-format (Google, 4 пробела, ширина 100)
- LICENSE (MIT)
- README с инструкцией сборки
- TODO.md
- заготовки тестов (unit с mocks)

Требования:
- SOLID, DI, без глобальных переменных.
- Комментарии на русском, объясняют "почему".
- Doxygen для публичных методов, шапка каждого файла.
- Naming: PascalCase / camelCase / m_ / UPPER_SNAKE.
- Вложенность ≤3, короткие функции.
- Нет магических чисел и хардкода пинов/топиков.
- Compile-time — config.h, runtime — NVS.

Содержимое TODO.md:
1. Опциональное шифрование ESP-NOW (LMK/PMK).
2. Тесты: выбрать фреймворк (Unity/Google Test), покрыть моками WifiManager/MqttManager/EspNowManager. Пока заготовки.
3. Заглушки-интерфейсы: OTA, Watchdog, SNTP time sync, TLS/secure boot/flash encryption.
4. Проверка на всех чипах (ESP32, S2, S3, C3).

Критерии приёмки:
- Gateway: captive portal, Wi-Fi reconnect, MQTT sub/pub, ESP-NOW rx/tx, мост ESP-NOW↔MQTT.
- Sensor: captive portal, лог, ESP-NOW, light sleep по таймеру, I2C/OneWire/SPI/UART.
- Device: captive portal, лог, ESP-NOW rx/tx, I2C/OneWire/SPI/UART.
- Все настройки — в config.h / NVS.
- Есть .clang-format, LICENSE, TODO.md, заготовки тестов.

Если чего-то не хватает — задай вопрос или пометь TODO, не додумывай.


## ПРОМПТ 3

Контекст: boilerplate `saspik-iot-sketches/esp32-boilerplate/` генерируется по ПРОМПТ 2 (архитектура — boilerplate-architecture.md). Часть модулей уже сгенерирована — ниже точный статус.

### УЖЕ СДЕЛАНО (не перегенерировать; править только при необходимости)

- Корень: `platformio.ini` (env gateway/sensor/device × esp32/s2/s3/c3 + native), `partitions.csv`, `.clang-format`, `LICENSE`.
- `include/config.h` — все compile-time константы: роль через `-DDEVICE_ROLE`, шифр `saspik-{роли}{протокол}{питание}{nnn}` (gateway=saspik-gwqm001, sensor=saspik-senb001, device=saspik-aenm001), пины `PIN_GPIO_1..5` (+`PIN_ONE_WIRE=27`), шины `BUS_*_ENABLED`/`PIN_*`, MQTT-дефолты и таймауты, `MQTT_PAYLOAD_MAX=128`, `MQTT_SUBSCRIPTIONS_MAX=8`, `MQTT_TOPIC_MAX_LEN=64`, ESP-NOW (канал/лимиты/типы 1..4/поля 0xFF,0xFE/FIELD_ID_*/`DEFAULT_GATEWAY_MAC`/`FIELD_DICTIONARY`/`NODE_TABLE`), интервалы, логирование (`LOG_*`), флаги примеров (`USE_MOCK_SENSOR`, `USE_EXAMPLE_*`).
- `lib/saspik-core` (include/saspik + src), всё по модулям:
  - core: `IComponent`, `DeviceInfo`, `TimerRegistry` (ClockFn, overflow-safe), `RuntimeConfig`;
  - config: `AppConfig`, `IConfigStore` (+`putUInt8`), `NvsConfigStore` (namespace `saspik_app`), `ConfigManager` (merge defaults←NVS);
  - log: `ILogSink`, `ILogger`, `Logger` (JSON-конверт `{level,src,event,msg,topic,unitId,objectId,uptime[,cause]}`), `UartLogSink`, `MqttLogSink` (кольцевая очередь drop-oldest), `EspNowLogSink`, `IRebootCauseStore`, `RebootCauseStore` (фасад над `DeviceLog`);
  - net: `TopicBuilder` (+`commandWildcardTopic` = `device/{unitId}/+`), `IMessageHandler`;
  - wifi: `IWifiManager`, `WifiManager` (reconnect по интервалу, авто-ребут по таймауту);
  - portal: `ICaptivePortal`, `CaptivePortal` (делегирует shared `WifiConfig` как есть);
  - mqtt: `MqttConfig`, `IMqttClient`, `MqttManager` (PubSubClient, PIMPL, LWT offline/online retained, resubscribe после reconnect, авто-ребут);
  - espnow: `EspNowTypes`, `IEspNowClient` (+`IEspNowListener`, `addPeer`), `EspNowCodec` (чистый), `EspNowManager` (RX-буфер однослотовый, доставка в update(), не в колбэке);
  - bridge: `EspNowMqttBridge` (чистый: DATA→sensor/{unitId}/{objectId}, LOG→.../log as-is, MQTT-команда→ESP-NOW COMMAND как JSON без разбора).

### ПРИНЯТЫЕ РЕШЕНИЯ (соблюдать)

- ESP-NOW протокол v2: `[type(1)][timestamp LE(4)][fieldId(1)+len(1)+data]*[crc8(1)]`; CRC-8/MAXIM (poly 0x31 отражённый → 0x8C, init 0x00); типы 1..4 = DATA/LOG/COMMAND/ACK; служебные поля `0xFF` (лог JSON), `0xFE` (команда JSON); лимит пакета 250 байт. Несовместим с v1 (node-sensor-1: без type/len) — миграция узлов на boilerplate-Sensor отдельная задача.
- Firmware-only `.cpp` обёрнуты в `#ifdef ARDUINO` (на хосте — пустая TU); заголовки чистые (Arduino-типы скрыты за PIMPL/интерфейсами).
- `static s_instance` только внутри `MqttManager`/`EspNowManager` (нужен для C-колбэков) — единственное задокументированное исключение из «без глобалов».
- `MqttConfig.clientId` — полный clientId (App собирает `CLIENT_ID_PREFIX` + `OBJECT_ID`); буфер PubSubClient = `LOG_ENVELOPE_MAX_LEN` (256), чтобы влезал конверт лога.
- env:native: `-DDEVICE_ROLE=2`, `-Iinclude`, `-Itest/native/mocks`, lib_deps только ArduinoJson.
- IntelliSense-ошибки про includePath (WString.h и т.п.) — ожидаемы, лечатся генерацией путей PlatformIO.

### ОСТАЛОСЬ СДЕЛАТЬ (генерировать по одному файлу)

1. Сон: `sys/ISleepManager.h`, `sys/LightSleepManager.h/.cpp` (esp_sleep_enable_timer_wakeup + esp_light_sleep_start; millis продолжается; `#ifdef ARDUINO`).
2. Шины: `interfaces/bus/IBus.h`, `I2cBus.h/.cpp`, `OneWireBus.h/.cpp`, `SpiBus.h/.cpp`, `UartBus.h/.cpp`, `InterfaceManager.h/.cpp` (BusConfig из config.h: `BUS_*_ENABLED`, `PIN_*`; неиспользуемые шины не создаются).
3. Интерфейсы периферии: `interfaces/sensor/ISensor.h` + `SensorSample.h`, `interfaces/actuator/IActuator.h`.
4. Приложения: `app/IApp.h`, `src/main.cpp` (диспетчеризация по `DEVICE_ROLE`), `GatewayApp`/`SensorApp`/`DeviceApp` (.h/.cpp) — Composition Root: DI-сборка модулей, Logger + sink'и, таймеры через `TimerRegistry`, неблокирующий loop, авто-ребут по таймаутам (кроме Sensor).
5. Примеры периферии (`src/app/examples/`): `Dht22Sensor` (`PIN_GPIO_3`, `USE_MOCK_SENSOR`), `FloatSensor` (`PIN_GPIO_4`, `FIELD_ID_FLOAT`), `RelayActuator` (`PIN_GPIO_5`) — реализуют `ISensor`/`IActuator`.
6. Тесты native: `test/native/mocks/` (`MockMqttClient`, `MockEspNowClient`, `MockWifiManager`, `MockConfigStore`, `MockLogSink`) + `test_*.cpp` (Unity): TopicBuilder, EspNowCodec (включая known-answer тест CRC-8/MAXIM), EspNowMqttBridge, ConfigManager, TimerRegistry.
7. Документы: `README.md` (сборка/загрузка/монитор, выбор шаблона, структура), `TODO.md` (по списку из ПРОМПТ 2), `library.json` для saspik-core.
8. Проверка: `pio test -e native` — зелёный; сборка `env:gateway_esp32` (далее s2/s3/c3) — без ошибок; исправить найденное.

Требования — те же, что в ПРОМПТ 2: SOLID/DI без глобалов, русские комментарии «почему», Doxygen, PascalCase/camelCase/m_/UPPER_SNAKE, вложенность ≤3, без магических чисел (всё из config.h), SPDX MIT.

Критерии приёмки:
- Gateway: captive portal, Wi-Fi reconnect, MQTT sub/pub, ESP-NOW rx/tx, мост ESP-NOW↔MQTT.
- Sensor: captive portal, лог, ESP-NOW, light sleep по таймеру, I2C/OneWire/SPI/UART.
- Device: captive portal, лог, ESP-NOW rx/tx, I2C/OneWire/SPI/UART.
- Все настройки — в config.h / NVS; host-тесты проходят в env:native.

Если чего-то не хватает — задай вопрос или пометь TODO, не додумывай.