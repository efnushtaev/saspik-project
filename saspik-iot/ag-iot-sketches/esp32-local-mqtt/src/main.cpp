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

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ========================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
DHT          dht(PIN_DHT, DHT_TYPE);

// Рабочий конфиг (WiFi + MQTT) — заполняется через captive portal / NVS
DeviceConfig config;

// Неблокирующий таймер
uint32_t lastSensorReadMs = 0;

// ======================== ПРОТОТИПЫ ========================

void connectMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void setLed(bool on);

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

    // Поддержание MQTT-соединения (каждый вызов loop)
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();

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

    Serial.print("Подключение к MQTT-брокеру: ");
    Serial.print(config.mqttHost);
    Serial.print(":");
    Serial.println(config.mqttPort);

    if (mqttClient.connect(clientId.c_str(), config.mqttUser, config.mqttPass)) {
        Serial.println("MQTT подключён.");

        // Подписка на топик управления светодиодом
        // Retained-сообщение будет доставлено сразу после подписки
        mqttClient.subscribe(TOPIC_SUBSCRIBE);
        Serial.print("Подписка на топик: ");
        Serial.println(TOPIC_SUBSCRIBE);
    } else {
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