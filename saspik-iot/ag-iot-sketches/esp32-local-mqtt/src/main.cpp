/*
 * ESP32 + DHT22 + MQTT — локальный мониторинг температуры/влажности
 * и управление встроенным светодиодом через MQTT.
 *
 * Плата:      ESP32 DevKit (esp32dev)
 * Датчик:     DHT22 на GPIO 4
 * Светодиод:  встроенный, GPIO 2 (active low — LOW = вкл, HIGH = выкл)
 * Framework:  Arduino (PlatformIO)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include "config.h"
#include "env_config.h"
#include "WifiConfig.h"
#include "DeviceLog.h"

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ========================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
DHT          dht(PIN_DHT, DHT_TYPE);

// Рабочий конфиг (WiFi + MQTT) — заполняется через captive portal / NVS
DeviceConfig config;

// Неблокирующий таймер
uint32_t lastSensorReadMs = 0;

// ======================== ДИАГНОСТИКА / УСТОЙЧИВОСТЬ ========================
// Параметры (таймауты, топик диагностики) заданы в config.h:
//   MQTT_REBOOT_TIMEOUT_MS, WIFI_REBOOT_TIMEOUT_MS,
//   WIFI_RECONNECT_INTERVAL_MS, DIAG_PUBLISH_INTERVAL_MS, TOPIC_DIAG

uint32_t mqttLostSinceMs = 0;      // момент начала непрерывной недоступности MQTT
bool     mqttWasConnected = false; // был ли MQTT подключён (для первого подключения)
uint32_t wifiLostSinceMs = 0;      // момент начала непрерывного обрыва WiFi
uint32_t lastWifiReconnectMs = 0;
uint32_t lastDiagPublishMs = 0;
bool     startupLogSent = false;   // отправлен ли хвост лога после старта

// ======================== ПРОТОТИПЫ ========================

void connectMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void setLed(bool on);
void handleWifiReconnect();
void checkMqttTimeout();
void sendDiagnostics();
void publishStartupLog();

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

    // Инициализация энергонезависимого лога (LittleFS) и причины ребута
    DeviceLog.begin();
    if (DeviceLog.hasRebootCause()) {
        Serial.print("[main] Предыдущий ребут по причине: ");
        Serial.println(DeviceLog.rebootCause());
    }
    DeviceLog.write("=== started, reason=%s ===",
                    DeviceLog.rebootCause()[0] ? DeviceLog.rebootCause() : "none");

    // Настройка пина светодиода (active low)
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);  // HIGH = выключен (active low)

    // Инициализация DHT
    dht.begin();

    // Настройка MQTT из конфига
    mqttClient.setServer(config.mqttHost, config.mqttPort);
    mqttClient.setCallback(callbackMQTT);

    Serial.print("Client ID: ");
    Serial.println("esp32-dht22-" + WiFi.macAddress());
}

// ======================== LOOP ========================

void loop() {
    // Обслуживание captive portal (no-op в штатном режиме)
    WifiConfig.handlePortal();

    // В режиме портала MQTT/сенсор не инициализированы — выходим
    if (WifiConfig.isPortalMode()) {
        return;
    }

    // Восстановление WiFi при обрыве (иначе MQTT никогда не переподключится)
    handleWifiReconnect();

    // Поддержание MQTT-соединения (каждый вызов loop)
    if (!mqttClient.connected()) {
        connectMQTT();
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
    uint32_t now = millis();
    if (now - lastSensorReadMs >= SENSOR_INTERVAL_MS) {
        lastSensorReadMs = now;
        publishSensorData();
    }
}

// ======================== MQTT ========================

void connectMQTT() {
    // Формируем clientId на основе MAC
    String clientId = "esp32-dht22-";
    clientId += WiFi.macAddress();
    clientId.replace(":", "");

    if (mqttClient.connect(clientId.c_str(), config.mqttUser, config.mqttPass)) {
        Serial.println("MQTT подключён.");
        mqttWasConnected = true;
        mqttLostSinceMs = 0; // сброс таймера

        // Подписка на топик управления светодиодом
        mqttClient.subscribe(TOPIC_SUBSCRIBE);
        DeviceLog.write("mqtt connected, subscribed %s", TOPIC_SUBSCRIBE);
    } else {
        // Фиксируем момент начала недоступности MQTT (только один раз за эпизод).
        // Не считаем сбоем первичное подключение при старте (mqttWasConnected == false).
        if (mqttLostSinceMs == 0 && mqttWasConnected) {
            mqttLostSinceMs = millis();
        }
        Serial.print("Ошибка MQTT, rc=");
        Serial.println(mqttClient.state());
    }
}

// ======================== CALLBACK MQTT ========================

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
    // Игнорируем сообщения не из нашего топика
    if (strcmp(topic, TOPIC_SUBSCRIBE) != 0) {
        return;
    }

    // Преобразуем payload в строку
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Получена команда LED: ");
    Serial.println(message);

    // Парсим JSON: {"state":"ON"} или {"state":"OFF"}
    StaticJsonDocument<32> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("Ошибка парсинга JSON: ");
        Serial.println(error.c_str());
        return;
    }

    const char* state = doc["state"];
    if (strcmp(state, "ON") == 0) {
        setLed(true);
    } else if (strcmp(state, "OFF") == 0) {
        setLed(false);
    } else {
        Serial.print("Неизвестная команда: ");
        Serial.println(state);
    }
}

// ======================== УПРАВЛЕНИЕ СВЕТОДИОДОМ ========================

void setLed(bool on) {
    // Active low: LOW = включено, HIGH = выключено
    digitalWrite(PIN_LED, on ? LOW : HIGH);
    Serial.print(on ? "Светодиод ВКЛЮЧЁН" : "Светодиод ВЫКЛЮЧЕН");
    Serial.println(" (GPIO 2)");
}

// ======================== ПУБЛИКАЦИЯ ДАННЫХ ДАТЧИКА ========================

void publishSensorData() {
    // Чтение температуры и влажности
    float humidity    = dht.readHumidity();
    float temperature = dht.readTemperature();  // °C

    // Проверка на ошибку чтения (nan)
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Ошибка чтения DHT22: получены некорректные данные (nan)");
        return;
    }

    // Формирование JSON с помощью ArduinoJson v6
    StaticJsonDocument<128> doc;
    doc["temperature"] = temperature;
    doc["humidity"]    = humidity;

    char jsonBuffer[128];
    size_t jsonLen = serializeJson(doc, jsonBuffer);

    // Полный топик: OBJECT_TYPE + UNIT_ID + OBJECT_ID
    String topic = String(OBJECT_TYPE) + '/' + String(UNIT_ID) + '/' + String(OBJECT_ID);

    // Публикация в MQTT
    bool published = mqttClient.publish(topic.c_str(), jsonBuffer, jsonLen);

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
    DeviceLog.write("wifi lost (status=%d), reconnecting", WiFi.status());
    WiFi.disconnect();
    WiFi.reconnect();

    // Резерв: если WiFi не поднялся за WIFI_REBOOT_TIMEOUT_MS (5 минут),
    // полный рестарт устройства (Wi-Fi повторного коннекта недостаточно).
    if (now - wifiLostSinceMs >= WIFI_REBOOT_TIMEOUT_MS) {
        DeviceLog.write("wifi unavailable >%lus, rebooting",
                        (unsigned long)(WIFI_REBOOT_TIMEOUT_MS / 1000));
        delay(100);
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

    // Периодическая публикация диагностики во время сбоя (для удалённого мониторинга)
    if (now - lastDiagPublishMs >= DIAG_PUBLISH_INTERVAL_MS) {
        lastDiagPublishMs = now;
        sendDiagnostics();
    }

    if (now - mqttLostSinceMs >= MQTT_REBOOT_TIMEOUT_MS) {
        DeviceLog.write("mqtt unavailable >%lus, rebooting",
                        (unsigned long)(MQTT_REBOOT_TIMEOUT_MS / 1000));
        delay(100); // дать записаться логу
        DeviceLog.setRebootCause("mqtt-timeout");
        ESP.restart();
    }
}

// ======================== ДИАГНОСТИКА В MQTT ========================

void sendDiagnostics() {
    if (!mqttClient.connected()) {
        return;
    }
    char msg[160];
    snprintf(msg, sizeof(msg),
        "{\"uptime\":%lu,\"wifi\":%d,\"mqtt\":%d,\"lostSec\":%lu}",
        (unsigned long)(millis() / 1000),
        WiFi.status(),
        mqttClient.state(),
        (unsigned long)((millis() - mqttLostSinceMs) / 1000));
    mqttClient.publish(TOPIC_DIAG, msg);
}

// ======================== ОТПРАВКА ХВОСТА ЛОГА ПРИ СТАРТЕ ========================

void publishStartupLog() {
    // Только если была зафиксирована причина предыдущего ребута
    if (!DeviceLog.hasRebootCause()) {
        return;
    }

    char tail[512];
    size_t n = DeviceLog.readTail(tail, sizeof(tail));
    if (n == 0) {
        return;
    }

    Serial.print("[main] Отправка хвоста лога (причина: ");
    Serial.print(DeviceLog.rebootCause());
    Serial.println(")");
    mqttClient.publish(TOPIC_DIAG, tail);

    // Причину сохраняем (по решению — не сбрасываем после отправки),
    // чтобы информация сохранялась до сброса/перезаписи.
}