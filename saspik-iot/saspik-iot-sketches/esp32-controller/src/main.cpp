#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>
#include "config.h"
#include <CRC8.h>
#include <ArduinoJson.h>
#include "mqtt_handler.h"
#include <time.h>
#include "WifiConfig.h"

CRC8 crc;

TaskHandle_t espNowTaskHandle;
TaskHandle_t mqttTaskHandle;

// Рабочий конфиг (WiFi + MQTT) — заполняется через captive portal / NVS
DeviceConfig config;

// ----------------------------------------------------------------------------
// WiFi
// ----------------------------------------------------------------------------

bool initWiFi()
{
  // Конфиг по умолчанию: подставляется в форму портала при пустом NVS
  DeviceConfig defaults = {};
  strncpy(defaults.wifiSsid, WIFI_SSID, sizeof(defaults.wifiSsid) - 1);
  strncpy(defaults.wifiPass, WIFI_PASS, sizeof(defaults.wifiPass) - 1);
  strncpy(defaults.mqttHost, MQTT_HOST, sizeof(defaults.mqttHost) - 1);
  defaults.mqttPort = MQTT_PORT;
  strncpy(defaults.mqttUser, MQTT_USER, sizeof(defaults.mqttUser) - 1);
  strncpy(defaults.mqttPass, MQTT_PASS, sizeof(defaults.mqttPass) - 1);

  // Старт: если true — подключены к WiFi, false — работает captive portal
  if (!WifiConfig.begin(config, CONFIG_BUTTON_PIN, &defaults))
  {
    return false;
  }

  WiFi.mode(WIFI_MODE_APSTA);

  Serial.printf("SSID: %s\n", config.wifiSsid);
  Serial.printf("Channel: %u\n", WiFi.channel());
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  return true;
}

// ----------------------------------------------------------------------------
// NTP
// ----------------------------------------------------------------------------

void initNTP()
{
  configTime(0, 0, NTP_SERVER);
  Serial.print("Syncing NTP");
  time_t now = time(nullptr);
  while (now < 100000)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" ok");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  char buf[32];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  Serial.printf("NTP time: %s\n", buf);
}

// ----------------------------------------------------------------------------
// ESP-NOW
// ----------------------------------------------------------------------------

typedef struct struct_message
{
  float TEMPERATURE;
  float HUMIDITY;
  float FLOAT_SENSOR;
  unsigned long TIMESTAMP;
} struct_message;

struct_message myData;

void onDataReceive(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  if (len < (int)(sizeof(unsigned long) + 1))
  {
    Serial.println("Not enough data");
    return;
  }

  uint8_t receivedCRC = incomingData[len - 1];

  crc.reset();
  crc.add(incomingData, len - 1);
  if (crc.calc() != receivedCRC)
  {
    Serial.println("CRC error");
    return;
  }

  unsigned long timestamp;
  memcpy(&timestamp, incomingData, sizeof(timestamp));
  myData.TIMESTAMP = timestamp;

  int offset = sizeof(timestamp);

  while (offset < len - 1)
  {
    if (offset + (int)sizeof(uint8_t) > len) break;

    uint8_t fieldId;
    memcpy(&fieldId, incomingData + offset, sizeof(fieldId));
    offset += sizeof(fieldId);

    if (fieldId == 1)
    {
      if (offset + (int)sizeof(float) > len) break;
      memcpy(&myData.TEMPERATURE, incomingData + offset, sizeof(float));
      offset += sizeof(float);
      Serial.printf("Temperature: %.2f\n", myData.TEMPERATURE);
    }
    else if (fieldId == 2)
    {
      if (offset + (int)sizeof(float) > len) break;
      memcpy(&myData.HUMIDITY, incomingData + offset, sizeof(float));
      offset += sizeof(float);
      Serial.printf("Humidity: %.2f\n", myData.HUMIDITY);
    }
    else if (fieldId == 3)
    {
      if (offset + (int)sizeof(float) > len) break;
      memcpy(&myData.FLOAT_SENSOR, incomingData + offset, sizeof(float));
      offset += sizeof(float);
      Serial.printf("Float sensor: %.2f\n", myData.FLOAT_SENSOR);
    }
    else
    {
      Serial.printf("Unknown fieldId: %u\n", fieldId);
      break;
    }
  }
}

void espNowTask(void *pvParameters)
{
  while (true)
  {
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void initEspNow()
{
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP NOW init failed");
    while (1);
  }

  esp_now_register_recv_cb(onDataReceive);
  xTaskCreatePinnedToCore(espNowTask, "ESP-NOW Task", 4096, NULL, 1, &espNowTaskHandle, 0);
}

// ----------------------------------------------------------------------------
// MQTT
// ----------------------------------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Connecting MQTT...");
    if (client.connect(CLIENT_ID, config.mqttUser, config.mqttPass))
    {
      Serial.println(" connected");
      client.subscribe(RELAY_TOPIC_LIGHT);
      client.subscribe(RELAY_TOPIC_HUMIDIFIER);
      client.subscribe(RELAY_TOPIC_FAN);
      client.subscribe(RELAY_TOPIC_WATER);
    }
    else
    {
      Serial.printf(" failed, rc=%d\n", client.state());
      delay(5000);
    }
  }
}

void getTimestampIso8601(char* buf, size_t len)
{
  time_t now = time(nullptr);
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  strftime(buf, len, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
}

void mqttTask(void *pvParameters)
{
  unsigned long lastPublish = 0;

  while (true)
  {
    if (!client.connected())
    {
      reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastPublish >= PUBLISH_INTERVAL_MS)
    {
      lastPublish = now;

      char ts[32];
      getTimestampIso8601(ts, sizeof(ts));

      JsonDocument doc;
      doc["temperature"] = myData.TEMPERATURE;
      doc["humidity"] = myData.HUMIDITY;
      doc["timestamp"] = ts;

      char buffer[256];
      serializeJson(doc, buffer);
      client.publish(SENSOR_TOPIC_DHT22, buffer);
      Serial.printf("-> %s: %s\n", SENSOR_TOPIC_DHT22, buffer);

      doc.clear();
      doc["floatSensor"] = myData.FLOAT_SENSOR;
      doc["timestamp"] = ts;

      serializeJson(doc, buffer);
      client.publish(SENSOR_TOPIC_FLOAT, buffer);
      Serial.printf("-> %s: %s\n", SENSOR_TOPIC_FLOAT, buffer);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initMqtt()
{
  client.setServer(config.mqttHost, config.mqttPort);
  client.setCallback(mqttCallback);
  xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 1, &mqttTaskHandle, 1);
}

// ----------------------------------------------------------------------------
// SETUP + LOOP
// ----------------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);
  delay(500);

  if (!initWiFi())
  {
    return;  // активен captive portal — loop() крутит handlePortal()
  }

  initNTP();
  initEspNow();
  initMqtt();

  pinMode(RELAY_LIGHT_D_PIN, OUTPUT);
  digitalWrite(RELAY_LIGHT_D_PIN, LOW);

  pinMode(RELAY_WATER_D_PIN, OUTPUT);
  digitalWrite(RELAY_WATER_D_PIN, LOW);

  pinMode(RELAY_FAN_D_PIN, OUTPUT);
  digitalWrite(RELAY_FAN_D_PIN, LOW);

  pinMode(RELAY_HUMIDIFIER_D_PIN, OUTPUT);
  digitalWrite(RELAY_HUMIDIFIER_D_PIN, LOW);
}

void loop() {
  WifiConfig.handlePortal();
  delay(10);
}
