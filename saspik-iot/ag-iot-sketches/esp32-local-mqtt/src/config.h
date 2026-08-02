#include <stdint.h>

// --- Класс устройства ---
constexpr char DEVICE_CLASS[] = "saspik.sa.wm.m001";

// --- Пины ---
constexpr uint8_t PIN_DHT       = 4;
constexpr uint8_t PIN_LED       = 2;

// --- Кнопка captive portal (внешняя, подтяжка к GND, INPUT_PULLUP, нажатие = LOW) ---
// При зажатии при включении открывается портал конфигурации WiFi/MQTT
// (даже если конфиг уже сохранён в NVS). Пин свободен: DHT=4, LED=2.
constexpr uint8_t CONFIG_BUTTON_PIN = 25;

// --- MQTT ---
constexpr char MQTT_BROKER[]      = "192.168.0.101";
constexpr uint16_t MQTT_PORT      = 1883;
constexpr char MQTT_USER[]        = "admin";
constexpr char MQTT_PASS[]        = "password123";
constexpr char TOPIC_PUBLISH[]    = "sensors/unitId2/dht22";
constexpr char TOPIC_SUBSCRIBE[]  = "led/control";

// --- Интервал чтения датчика (мс) ---
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;

// --- Тип датчика DHT (DHT22 = 22) ---
constexpr uint8_t DHT_TYPE = 22;
