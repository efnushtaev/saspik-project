/*
 * ESP32 + MQTT — конфигурируемые сенсоры и автоматы.
 *
 * Плата:      ESP32 DevKit (esp32dev)
 * Каналы:     CHANNELS[] в config.h: имя, пин, роль, модель.
 * Управление: device/{unitId}/{objectId}/command
 *             {"channel":"<имя>","value":"ON"|"OFF"}
 * Состояние:  retained device/{unitId}/{objectId}/state
 * Framework:  Arduino (PlatformIO)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <math.h>
#include <strings.h>
#include "config.h"
#include "env_config.h"
#include "WifiConfig.h"
#include "DeviceLog.h"

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ========================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// Рабочий конфиг (WiFi + MQTT) — заполняется через captive portal / NVS
DeviceConfig config;

// Неблокирующий таймер
uint32_t lastSensorReadMs = 0;

// Каналы, собранные из CHANNELS[] в config.h.
struct SensorRuntime {
    const ChannelConfig* cfg;
    DHT* dht;  // только для ChannelModel::Dht22
};
struct ActuatorRuntime {
    const ChannelConfig* cfg;
    bool on;
};
SensorRuntime sensors[MAX_CHANNELS];
size_t sensorCount = 0;
ActuatorRuntime actuators[MAX_CHANNELS];
size_t actuatorCount = 0;

// ======================== ДИАГНОСТИКА / УСТОЙЧИВОСТЬ ========================
// Параметры (таймауты, топик диагностики) заданы в config.h:
//   MQTT_REBOOT_TIMEOUT_MS, WIFI_REBOOT_TIMEOUT_MS,
//   WIFI_RECONNECT_INTERVAL_MS, MQTT_CONNECT_TIMEOUT_MS, TOPIC_DIAG

uint32_t mqttLostSinceMs = 0;      // момент начала непрерывной недоступности MQTT
bool     mqttWasConnected = false; // был ли MQTT подключён (для первого подключения)
uint32_t mqttConnectErrors = 0;    // неудачных попыток подключения за текущий эпизод
uint32_t wifiLostSinceMs = 0;      // момент начала непрерывного обрыва WiFi
uint32_t lastWifiReconnectMs = 0;
uint32_t lastMqttConnectMs = 0;    // момент последней попытки подключения к MQTT
bool     startupLogSent = false;   // отправлен ли хвост лога после старта

// ======================== ПРОТОТИПЫ ========================

void initChannels();
void connectMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void publishState();
ActuatorRuntime* findActuator(const char* key);
void setActuator(ActuatorRuntime& actuator, bool on);
bool parseBoolValue(const char* value, bool& out);
void handleWifiReconnect();
void checkMqttTimeout();
void publishStartupLog();
void publishLogEvent(const char* level, const char* event, const char* msg);
void publishReconnectLog(uint32_t downSec, uint32_t errors);

// ======================== SETUP ========================

void setup() {
    Serial.begin(115200);

    // Конфиг по умолчанию: подставляется в форму портала при пустом NVS
    DeviceConfig defaults = {};
    strncpy(defaults.wifiSsid, WIFI_SSID, sizeof(defaults.wifiSsid) - 1);
    strncpy(defaults.wifiPass, WIFI_PASS, sizeof(defaults.wifiPass) - 1);
    strncpy(defaults.mqttHost, MQTT_BROKER, sizeof(defaults.mqttHost) - 1);
    defaults.mqttPort = MQTT_PORT;
    strncpy(defaults.mqttUser, MQTT_USER, sizeof(defaults.mqttUser) - 1);
    strncpy(defaults.mqttPass, MQTT_PASS, sizeof(defaults.mqttPass) - 1);

    // Старт: если true — подключены к WiFi, false — работает captive portal
    if (!WifiConfig.begin(config, CONFIG_BUTTON_PIN, &defaults)) {
        return;
    }
    Serial.printf("[main] setup: WiFi ok at %lums\n", (unsigned long)millis());

    // Инициализация причины ребута (NVS); файловый лог не используется
    DeviceLog.begin();
    Serial.printf("[main] setup: DeviceLog ok at %lums\n", (unsigned long)millis());
    if (DeviceLog.hasRebootCause()) {
        Serial.print("[main] Предыдущий ребут по причине: ");
        Serial.println(DeviceLog.rebootCause());
    }
    Serial.printf("[main] started, причиной reboot: %s\n",
                  DeviceLog.rebootCause()[0] ? DeviceLog.rebootCause() : "none");

    initChannels();

    // Настройка MQTT из конфига
    mqttClient.setServer(config.mqttHost, config.mqttPort);
    mqttClient.setCallback(callbackMQTT);
    // Короткий таймаут TCP, чтобы connect() не блокировал loop() дольше
    // таймаута Task WDT (иначе возникает бесконечный rst:0x8 при недоступном брокере).
    wifiClient.setTimeout(MQTT_CONNECT_TIMEOUT_MS / 1000);
    mqttClient.setSocketTimeout(MQTT_CONNECT_TIMEOUT_MS / 1000);

    Serial.print("Client ID: ");
    Serial.println("esp32-device-" + WiFi.macAddress());
    Serial.printf("[main] setup: done at %lums, entering loop\n", (unsigned long)millis());
}

// ======================== LOOP ========================

void loop() {
    // Кормим Task WDT и даём время фоновым задачам (WiFi/стека).
    // Без этого длительные операции (connect и т.п.) вызывают rst:0x8 reset.
    yield();

    // Обслуживание captive portal (no-op в штатном режиме)
    WifiConfig.handlePortal();

    // В режиме портала MQTT/сенсор не инициализированы — выходим
    if (WifiConfig.isPortalMode()) {
        return;
    }

    // Восстановление WiFi при обрыве (иначе MQTT никогда не переподключится)
    handleWifiReconnect();

    uint32_t now = millis();
    // Поддержание MQTT-соединения, но не чаще одного раза в интервал:
    // connect() блокирует до MQTT_CONNECT_TIMEOUT_MS, и без этого может
    // удерживать loop() дольше таймаута Task WDT (недоступный брокер -> rst:0x8).
    if (!mqttClient.connected()) {
        if (now - lastMqttConnectMs >= MQTT_RECONNECT_INTERVAL_MS) {
            lastMqttConnectMs = now;
            connectMQTT();
        }
    } else {
        // Успешное соединение — сбрасываем таймер "недоступности"
        mqttLostSinceMs = 0;
    }
    mqttClient.loop();

    // Авто-ребут при длительной недоступности MQTT
    checkMqttTimeout();

    // Отправка хвоста лога после успешного стартового подключения
    if (!startupLogSent && mqttClient.connected()) {
        publishStartupLog();
        startupLogSent = true;
    }

    // Неблокирующая отправка данных по таймеру
    if (now - lastSensorReadMs >= SENSOR_INTERVAL_MS) {
        lastSensorReadMs = now;
        publishSensorData();
    }
}

// ======================== MQTT ========================

void connectMQTT() {
    // Формируем clientId на основе MAC
    String clientId = "esp32-device-";
    clientId += WiFi.macAddress();
    clientId.replace(":", "");

    // LWT: при обрыве соединения (таймаут keepalive, пропажа питания/связи)
    // брокер сам публикует retained {"status":"offline"} в TOPIC_STATUS
    constexpr char WILL_OFFLINE[] = "{\"status\":\"offline\"}";
    if (mqttClient.connect(clientId.c_str(), config.mqttUser, config.mqttPass,
                           TOPIC_STATUS, 0, true, WILL_OFFLINE)) {
        Serial.println("MQTT подключён.");
        mqttWasConnected = true;

        // Retained-статус online для сервера/UI (перезаписывает offline от LWT)
        mqttClient.publish(TOPIC_STATUS, "{\"status\":\"online\"}", true);

        // Retained-состояние всех автоматов: после ребута актуализирует
        // Influx/UI фактическим состоянием пинов (стартуют выключенными).
        publishState();

        // Один /command на объект; имя канала передаётся в payload.
        mqttClient.subscribe(TOPIC_COMMAND);
        Serial.printf("mqtt connected, subscribed %s\n", TOPIC_COMMAND);

        // Событие восстановления после обрыва — до сброса таймера недоступности
        // (downSec — длительность обрыва, errors — число неудачных попыток).
        // При первом подключении на старте — обычное событие connect.
        if (mqttLostSinceMs != 0) {
            publishReconnectLog((millis() - mqttLostSinceMs) / 1000,
                                mqttConnectErrors);
        } else {
            publishLogEvent("info", "connect", "mqtt connected");
        }
        mqttLostSinceMs = 0;     // сброс таймера
        mqttConnectErrors = 0;   // счётчик попыток отработал — обнуляем
    } else {
        // Фиксируем момент начала недоступности MQTT (только один раз за эпизод).
        // Не считаем сбоем первичное подключение при старте (mqttWasConnected == false).
        if (mqttLostSinceMs == 0 && mqttWasConnected) {
            mqttLostSinceMs = millis();
        }
        ++mqttConnectErrors;
        Serial.print("Ошибка MQTT, rc=");
        Serial.println(mqttClient.state());
    }
}

// ======================== CALLBACK MQTT ========================

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
    // Один командный топик на объект; имена каналов — в JSON.
    if (strcmp(topic, TOPIC_COMMAND) != 0) {
        return;
    }

    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Получена команда: ");
    Serial.println(message);

    // {"channel":"<любое имя>","value":"ON"|"OFF"}
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("Ошибка парсинга JSON: ");
        Serial.println(error.c_str());
        return;
    }

    const char* channel = doc["channel"] | "";
    const char* value = doc["value"] | "";
    if (channel[0] == '\0' || value[0] == '\0') {
        Serial.println("Команда без channel/value — игнорируется");
        return;
    }

    ActuatorRuntime* actuator = findActuator(channel);
    if (actuator == nullptr) {
        Serial.printf("Неизвестный канал: %s\n", channel);
        return;
    }

    bool on = false;
    if (!parseBoolValue(value, on)) {
        Serial.printf("Неизвестное значение: %s\n", value);
        return;
    }
    setActuator(*actuator, on);
}

// ======================== КАНАЛЫ ========================

void initChannels() {
    sensorCount = 0;
    actuatorCount = 0;

    for (size_t i = 0; i < CHANNEL_COUNT && i < MAX_CHANNELS; ++i) {
        const ChannelConfig& cfg = CHANNELS[i];
        if (cfg.role == ChannelRole::Sensor) {
            SensorRuntime& runtime = sensors[sensorCount++];
            runtime.cfg = &cfg;
            runtime.dht = nullptr;
            if (cfg.model == ChannelModel::Dht22) {
                runtime.dht = new DHT(cfg.pin, DHT_TYPE);
                if (runtime.dht != nullptr) {
                    runtime.dht->begin();
                }
            } else if (cfg.model == ChannelModel::Float) {
                pinMode(cfg.pin, INPUT_PULLUP);
            }
            Serial.printf("channel %s: sensor on GPIO %u\n", cfg.key, cfg.pin);
        } else {
            ActuatorRuntime& runtime = actuators[actuatorCount++];
            runtime.cfg = &cfg;
            runtime.on = false;
            pinMode(cfg.pin, OUTPUT);
            digitalWrite(cfg.pin, cfg.activeLow ? HIGH : LOW);
            Serial.printf("channel %s: actuator on GPIO %u\n", cfg.key, cfg.pin);
        }
    }
}

ActuatorRuntime* findActuator(const char* key) {
    for (size_t i = 0; i < actuatorCount; ++i) {
        if (strcmp(actuators[i].cfg->key, key) == 0) {
            return &actuators[i];
        }
    }
    return nullptr;
}

bool parseBoolValue(const char* value, bool& out) {
    if (strcasecmp(value, "ON") == 0 || strcmp(value, "1") == 0 ||
        strcasecmp(value, "TRUE") == 0) {
        out = true;
        return true;
    }
    if (strcasecmp(value, "OFF") == 0 || strcmp(value, "0") == 0 ||
        strcasecmp(value, "FALSE") == 0) {
        out = false;
        return true;
    }
    return false;
}

void setActuator(ActuatorRuntime& actuator, bool on) {
    actuator.on = on;
    const ChannelConfig& cfg = *actuator.cfg;
    if (cfg.activeLow) {
        digitalWrite(cfg.pin, on ? LOW : HIGH);
    } else {
        digitalWrite(cfg.pin, on ? HIGH : LOW);
    }
    Serial.printf("channel %s: %s (GPIO %u)\n", cfg.key, on ? "ON" : "OFF",
                  cfg.pin);
    publishState();
}

// Retained-карта всех автоматов: {"канал":"ON","другой":"OFF"}.
void publishState() {
    if (!mqttClient.connected()) {
        return;
    }
    StaticJsonDocument<512> doc;
    for (size_t i = 0; i < actuatorCount; ++i) {
        doc[actuators[i].cfg->key] = actuators[i].on ? "ON" : "OFF";
    }
    char jsonBuffer[512];
    const size_t jsonLen = serializeJson(doc, jsonBuffer);
    mqttClient.publish(TOPIC_STATE, reinterpret_cast<const uint8_t*>(jsonBuffer),
                       jsonLen, true);
}

// ======================== ПУБЛИКАЦИЯ ДАННЫХ ДАТЧИКА ========================

void publishSensorData() {
    if (sensorCount == 0) {
        return;
    }

    StaticJsonDocument<512> doc;
    const float t = static_cast<float>(millis()) / 1000.0f;

    for (size_t i = 0; i < sensorCount; ++i) {
        const ChannelConfig& cfg = *sensors[i].cfg;
        if (cfg.model == ChannelModel::Dht22) {
            float temperature;
            float humidity;
            if (cfg.mock) {
                temperature = 23.0f + 3.0f * sinf(t * 0.1f);
                humidity = 45.0f + 8.0f * sinf(t * 0.07f + 2.0f);
            } else {
                if (sensors[i].dht == nullptr) {
                    continue;
                }
                humidity = sensors[i].dht->readHumidity();
                temperature = sensors[i].dht->readTemperature();
                if (isnan(humidity) || isnan(temperature)) {
                    Serial.printf("channel %s: DHT read failed\n", cfg.key);
                    continue;
                }
            }
            doc[cfg.key] = temperature;
            if (cfg.key2 != nullptr) {
                doc[cfg.key2] = humidity;
            }
        } else if (cfg.model == ChannelModel::Float) {
            const bool on = cfg.mock
                                ? (static_cast<uint32_t>(millis() / 5000) % 2 == 0)
                                : (digitalRead(cfg.pin) == LOW);
            doc[cfg.key] = on ? 1.0f : 0.0f;
        }
    }

    if (doc.size() == 0) {
        return;
    }

    char jsonBuffer[512];
    const size_t jsonLen = serializeJson(doc, jsonBuffer);
    String topic = String(OBJECT_TYPE) + '/' + String(UNIT_ID) + '/' + String(OBJECT_ID);

    const bool published = mqttClient.publish(
        topic.c_str(), reinterpret_cast<const uint8_t*>(jsonBuffer), jsonLen, false);
    if (published) {
        Serial.print("Опубликовано в топик \"");
        Serial.print(topic);
        Serial.print("\": ");
        Serial.println(jsonBuffer);
    } else {
        Serial.println("Ошибка публикации MQTT");
    }
}

// ======================== ВОССТАНОВЛЕНИЕ WIFI ========================

void handleWifiReconnect() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiLostSinceMs = 0;
        return;
    }

    uint32_t now = millis();
    // Не чаще одного раза в WIFI_RECONNECT_INTERVAL_MS
    if (now - lastWifiReconnectMs < WIFI_RECONNECT_INTERVAL_MS) {
        return;
    }
    lastWifiReconnectMs = now;

    if (wifiLostSinceMs == 0) {
        wifiLostSinceMs = now;
    }
    Serial.printf("wifi lost (status=%d), reconnecting\n", WiFi.status());
    WiFi.disconnect();
    WiFi.reconnect();

    // Резерв: если WiFi не поднялся за WIFI_REBOOT_TIMEOUT_MS (5 минут),
    // полный рестарт устройства (Wi-Fi повторного коннекта недостаточно).
    if (now - wifiLostSinceMs >= WIFI_REBOOT_TIMEOUT_MS) {
        Serial.printf("wifi unavailable >%lus, rebooting\n",
                      (unsigned long)(WIFI_REBOOT_TIMEOUT_MS / 1000));
        DeviceLog.savePrevUptime(); // prevUptime уйдёт в startup-конверт после ребута
        DeviceLog.setRebootCause("wifi-lost");
        ESP.restart();
    }
}

// ======================== АВТО-РЕБУТ ПРИ ДОЛГОЙ НЕДОСТУПНОСТИ MQTT ========================

void checkMqttTimeout() {
    if (mqttLostSinceMs == 0) {
        return;
    }
    uint32_t now = millis();

    if (now - mqttLostSinceMs >= MQTT_REBOOT_TIMEOUT_MS) {
        Serial.printf("mqtt unavailable >%lus, rebooting\n",
                      (unsigned long)(MQTT_REBOOT_TIMEOUT_MS / 1000));
        DeviceLog.savePrevUptime(); // prevUptime уйдёт в startup-конверт после ребута
        DeviceLog.setRebootCause("mqtt-timeout");
        ESP.restart();
    }
}

// ======================== ДИАГНОСТИКА В MQTT ========================

// Публикует единый JSON-конверт лога в топик TOPIC_DIAG:
//   {"level","src":"device","event","msg","topic","unitId","objectId","uptime"[,"cause"]}
void publishLogEvent(const char* level, const char* event, const char* msg) {
    if (!mqttClient.connected()) {
        return;
    }
    StaticJsonDocument<384> doc;
    doc["level"]    = level;
    doc["src"]      = "device";
    doc["event"]    = event;
    doc["msg"]      = msg;
    doc["topic"]    = TOPIC_DIAG;
    doc["unitId"]   = UNIT_ID;
    doc["objectId"] = OBJECT_ID;
    doc["uptime"]   = (unsigned long)(millis() / 1000);

    char jsonBuffer[384];
    size_t jsonLen = serializeJson(doc, jsonBuffer);
    mqttClient.publish(TOPIC_DIAG, reinterpret_cast<const uint8_t*>(jsonBuffer),
                       jsonLen, false);
}

// Событие восстановления MQTT после обрыва:
//   downSec — длительность недоступности (с), errors — число неудачных попыток.
// Публикуется до сброса mqttLostSinceMs, иначе downSec будет равен 0.
void publishReconnectLog(uint32_t downSec, uint32_t errors) {
    if (!mqttClient.connected()) {
        return;
    }
    StaticJsonDocument<384> doc;
    doc["level"]    = "info";
    doc["src"]      = "device";
    doc["event"]    = "reconnect";
    doc["msg"]      = "mqtt reconnected";
    doc["topic"]    = TOPIC_DIAG;
    doc["unitId"]   = UNIT_ID;
    doc["objectId"] = OBJECT_ID;
    doc["uptime"]   = (unsigned long)(millis() / 1000);
    doc["downSec"]  = downSec;
    doc["errors"]   = errors;

    char jsonBuffer[384];
    size_t jsonLen = serializeJson(doc, jsonBuffer);
    mqttClient.publish(TOPIC_DIAG, reinterpret_cast<const uint8_t*>(jsonBuffer),
                       jsonLen, false);
}

// ======================== СТАРТОВЫЙ КОНВЕРТ (ПРИЧИНА РЕБУТА) ========================

void publishStartupLog() {
    // Только если была зафиксирована причина предыдущего ребута
    if (!DeviceLog.hasRebootCause()) {
        return;
    }

    const char* cause = DeviceLog.rebootCause();

    Serial.print("[main] Стартовый конверт, причина ребута: ");
    Serial.println(cause);

    // Единый конверт: event = "startup", причина ребута — в поле cause,
    // длительность сессии до ребута — в поле prevUptime (0/нет — питания
    // выключали, а не авто-ребут).
    StaticJsonDocument<384> doc;
    doc["level"]    = "info";
    doc["src"]      = "device";
    doc["event"]    = "startup";
    doc["msg"]      = "startup after reboot";
    doc["topic"]    = TOPIC_DIAG;
    doc["unitId"]   = UNIT_ID;
    doc["objectId"] = OBJECT_ID;
    doc["uptime"]   = (unsigned long)(millis() / 1000);
    doc["cause"]    = cause;
    if (DeviceLog.prevUptime() > 0) {
        doc["prevUptime"] = DeviceLog.prevUptime();
    }

    char jsonBuffer[384];
    size_t jsonLen = serializeJson(doc, jsonBuffer);
    mqttClient.publish(TOPIC_DIAG, reinterpret_cast<const uint8_t*>(jsonBuffer),
                       jsonLen, false);

    // Конверт отправлен — стираем cause и prevUptime из NVS, иначе после
    // обычной (не авто-) перезагрузки startup повторно ушёл бы с ложным cause.
    DeviceLog.clearRebootData();
}
