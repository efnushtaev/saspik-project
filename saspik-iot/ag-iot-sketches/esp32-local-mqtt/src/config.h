#include <stdint.h>

// --- Параметры объекта ---
constexpr char OBJECT_ID[] = "saspik.sa.wm.m002";
constexpr char UNIT_ID[] = "unitId2";
constexpr char OBJECT_TYPE[] = "sensor";

// --- Пины ---
constexpr uint8_t PIN_DHT       = 4;
constexpr uint8_t PIN_LED       = 2;

// --- Кнопка captive portal (внешняя, подтяжка к GND, INPUT_PULLUP, нажатие = LOW) ---
// При зажатии при включении открывается портал конфигурации WiFi/MQTT
// (даже если конфиг уже сохранён в NVS). Пин свободен: DHT=4, LED=2.
constexpr uint8_t CONFIG_BUTTON_PIN = 25;

// --- MQTT ---
constexpr char MQTT_BROKER[]        = "192.168.0.118";
constexpr uint16_t MQTT_PORT        = 1883;
constexpr char MQTT_USER[]          = "admin";
constexpr char MQTT_PASS[]          = "password123";
constexpr char TOPIC_SUBSCRIBE[]    = "device/unitId2/saspik.sa.wm.m002";

// --- Диагностика и устойчивость ---
// Топик для публикации хвоста лога и диагностики (управляется через MQTT)
constexpr char TOPIC_DIAG[]         = "device/unitId2/saspik.sa.wm.m002/log";
// Время непрерывной недоступности MQTT, после которого авто-ребут (мс)
constexpr uint32_t MQTT_REBOOT_TIMEOUT_MS = 900000UL; // 15 минут
// Время непрерывной недоступности WiFi, после которого авто-ребут (мс)
constexpr uint32_t WIFI_REBOOT_TIMEOUT_MS = 300000UL; // 5 минут
// Минимальный интервал между попытками восстановления WiFi (мс)
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000UL;
// Минимальный интервал между попытками подключения к MQTT (мс).
// Предотвращает блокировку loop() на connect при недоступном брокере
// (что иначе вызывает Task WDT reset, rst:0x8).
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 3000UL;
// Таймаут TCP-соединения к брокеру (мс) — чтобы connect не висел дольше WDT.
constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS   = 4000UL;
// Интервал публикации диагностики в MQTT во время длительного сбоя (мс)
constexpr uint32_t DIAG_PUBLISH_INTERVAL_MS = 60000UL;

// --- Интервал чтения датчика (мс) ---
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;

// --- Тип датчика DHT (DHT22 = 22) ---
constexpr uint8_t DHT_TYPE = 22;

// --- Мок датчика ---
// true — генерировать данные псевдослучайно (физический DHT22 не подключён),
// false — читать реальный датчик.
constexpr bool USE_MOCK_SENSOR = true;
