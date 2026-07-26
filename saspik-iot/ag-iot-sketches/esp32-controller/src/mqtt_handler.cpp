#include "mqtt_handler.h"
#include "config.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>

extern PubSubClient client;

void controlRelay(int relayPin, const char* relayName, const char* state)
{
  if (strcmp(state, "ON") == 0 || strcmp(state, "1") == 0)
  {
    digitalWrite(relayPin, HIGH);
    Serial.printf("Relay %s ON\n", relayName);
  }
  else if (strcmp(state, "OFF") == 0 || strcmp(state, "0") == 0)
  {
    digitalWrite(relayPin, LOW);
    Serial.printf("Relay %s OFF\n", relayName);
  }
  else
  {
    Serial.printf("Invalid state for relay %s: %s\n", relayName, state);
  }
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  String message = String((char *)payload, length);

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  const char* state = nullptr;

  if (!error)
  {
    if (doc.is<const char*>())
    {
      state = doc.as<const char*>();
    }
    else if (doc["state"].is<const char*>())
    {
      state = doc["state"];
    }
  }

  if (state == nullptr)
  {
    state = message.c_str();
  }

  if (String(topic) == RELAY_TOPIC_LIGHT)
  {
    controlRelay(RELAY_LIGHT_D_PIN, "light", state);
  }
  else if (String(topic) == RELAY_TOPIC_HUMIDIFIER)
  {
    controlRelay(RELAY_HUMIDIFIER_D_PIN, "humidifier", state);
  }
  else if (String(topic) == RELAY_TOPIC_FAN)
  {
    controlRelay(RELAY_FAN_D_PIN, "fan", state);
  }
  else if (String(topic) == RELAY_TOPIC_WATER)
  {
    controlRelay(RELAY_WATER_D_PIN, "water", state);
  }
}
