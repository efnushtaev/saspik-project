#pragma once

#include <stddef.h>
#include <stdint.h>

// --- Параметры объекта ---
// Новое устройство автоматики. Старый saspik.sa.wm.m003 остаётся на своей
// прошивке и в этом конфиге не участвует.
constexpr char OBJECT_ID[] = "saspik-awqm001";
constexpr char UNIT_ID[]   = "unitId3";
constexpr char OBJECT_TYPE[] = "sensor";  // префикс топика данных

// --- Роли и модели пользовательских каналов ---
// Пин выбирается здесь, имя канала — здесь и в UI (spec.key).
enum class ChannelRole : uint8_t { Sensor, Actuator };
enum class ChannelModel : uint8_t { Dht22, Float, Relay, Led };

struct ChannelConfig {
    const char* key;       // произвольное имя канала
    const char* key2;      // второй ключ DHT22, иначе nullptr
    uint8_t pin;           // пользовательский GPIO
    ChannelRole role;
    ChannelModel model;
    bool activeLow;        // для LED/реле
    bool mock;             // для сенсоров: генерировать значения без железа
};

// Пример: два автомата на одном объекте. Имена и пины меняются пользователем.
// 13 и 26 — пользовательские GPIO; 2/4/25 в этом примере не заняты.
constexpr ChannelConfig CHANNELS[] = {
    {"свет",  nullptr, 13, ChannelRole::Actuator, ChannelModel::Relay, false, false},
    {"полив", nullptr, 26, ChannelRole::Actuator, ChannelModel::Relay, false, false},
};
constexpr size_t MAX_CHANNELS = 8;
constexpr size_t CHANNEL_COUNT = sizeof(CHANNELS) / sizeof(CHANNELS[0]);

// --- Системные пины ---
// Кнопка captive portal (внешняя, подтяжка к GND, INPUT_PULLUP, нажатие = LOW).
constexpr uint8_t CONFIG_BUTTON_PIN = 25;

// --- MQTT ---
constexpr char MQTT_BROKER[]        = "192.168.0.118";
constexpr uint16_t MQTT_PORT        = 1883;
constexpr char MQTT_USER[]          = "admin";
constexpr char MQTT_PASS[]          = "password123";

constexpr char TOPIC_COMMAND[]      = "device/unitId3/saspik-awqm001/command";
constexpr char TOPIC_STATE[]        = "device/unitId3/saspik-awqm001/state";
constexpr char TOPIC_STATUS[]       = "device/unitId3/saspik-awqm001/status";
constexpr char TOPIC_DIAG[]         = "device/unitId3/saspik-awqm001/log";

// --- Диагностика и устойчивость ---
constexpr uint32_t MQTT_REBOOT_TIMEOUT_MS = 900000UL; // 15 минут
constexpr uint32_t WIFI_REBOOT_TIMEOUT_MS = 300000UL; // 5 минут
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000UL;
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 3000UL;
constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS   = 4000UL;

// --- Интервал чтения датчиков (мс) ---
constexpr uint32_t SENSOR_INTERVAL_MS = 2000;

// --- Тип DHT (DHT22 = 22) ---
constexpr uint8_t DHT_TYPE = 22;
