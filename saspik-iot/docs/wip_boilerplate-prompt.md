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