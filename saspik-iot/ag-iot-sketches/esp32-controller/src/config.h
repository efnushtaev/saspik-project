#ifndef CONFIG_H
#define CONFIG_H

// Кнопка captive portal (внешняя, подтяжка к GND, INPUT_PULLUP, нажатие = LOW)
// При зажатии при включении открывается портал конфигурации WiFi/MQTT.
// Пин свободен: реле 12/13/14/27.
#define CONFIG_BUTTON_PIN 32

// WiFi (дефолт, переопределяется через captive portal / NVS)
#define WIFI_SSID "TP-Link_B354"
#define WIFI_PASS "57339016"

// MQTT (кластер)
#define MQTT_HOST "185.72.145.19"
#define MQTT_PORT 1883
#define MQTT_USER "admin"
#define MQTT_PASS "password123"
#define CLIENT_ID "esp32-controller"

// Unit
#define UNIT_ID "unitId1"

// Топики сенсоров
#define SENSOR_TOPIC_DHT22 "sensor/" UNIT_ID "/dht22"
#define SENSOR_TOPIC_FLOAT "sensor/" UNIT_ID "/float-1"

// Топики команд реле
#define RELAY_TOPIC_LIGHT "units/" UNIT_ID "/commands/a_relay1"
#define RELAY_TOPIC_HUMIDIFIER "units/" UNIT_ID "/commands/a_relay2"
#define RELAY_TOPIC_FAN "units/" UNIT_ID "/commands/a_relay3"
#define RELAY_TOPIC_WATER "units/" UNIT_ID "/commands/a_relay4"

// Пины реле
#define RELAY_LIGHT_D_PIN 27
#define RELAY_WATER_D_PIN 14
#define RELAY_FAN_D_PIN 12
#define RELAY_HUMIDIFIER_D_PIN 13

// Интервал публикации (мс)
#define PUBLISH_INTERVAL_MS 5000

// NTP
#define NTP_SERVER "pool.ntp.org"

#endif
