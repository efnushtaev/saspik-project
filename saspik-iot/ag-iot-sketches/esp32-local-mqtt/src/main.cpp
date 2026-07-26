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

// ======================== ГЛОБАЛЬНЫЕ ОБЪЕКТЫ ========================

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);
DHT          dht(PIN_DHT, DHT_TYPE);

// Неблокирующий таймер
uint32_t lastSensorReadMs = 0;

// ======================== ПРОТОТИПЫ ========================

void connectWiFi();
void connectMQTT();
void callbackMQTT(char* topic, byte* payload, unsigned int length);
void publishSensorData();
void setLed(bool on);

// ======================== SETUP ========================

void setup() {
    Serial.begin(115200);

    // Настройка пина светодиода (active low)
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);  // HIGH = выключен (active low)

    // Инициализация DHT
    dht.begin();

    // Генерация уникального clientId на основе MAC-адреса
    String clientId = "esp32-dht22-";
    clientId += WiFi.macAddress();
    clientId.replace(":", "");

    // Настройка MQTT
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(callbackMQTT);

    // Подключение к WiFi
    connectWiFi();

    Serial.print("Client ID: ");
    Serial.println(clientId);
}

// ======================== LOOP ========================

void loop() {
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

// ======================== WiFi ========================

void connectWiFi() {
    Serial.print("Подключение к WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Неблокирующее ожидание — проверяем статус в цикле
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);                     // краткая задержка допустима только в setup
        Serial.print(".");
    }

    Serial.println();
    Serial.print("WiFi подключён. IP: ");
    Serial.println(WiFi.localIP());
}

// ======================== MQTT ========================

void connectMQTT() {
    // Формируем clientId на основе MAC
    String clientId = "esp32-dht22-";
    clientId += WiFi.macAddress();
    clientId.replace(":", "");

    Serial.print("Подключение к MQTT-брокеру: ");
    Serial.print(MQTT_BROKER);
    Serial.print(":");
    Serial.println(MQTT_PORT);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
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

    // Публикация в MQTT
    bool published = mqttClient.publish(TOPIC_PUBLISH, jsonBuffer, jsonLen);

    if (published) {
        Serial.print("Опубликовано в топик \"");
        Serial.print(TOPIC_PUBLISH);
        Serial.print("\": ");
        Serial.println(jsonBuffer);
    } else {
        Serial.println("Ошибка публикации MQTT");
    }
}