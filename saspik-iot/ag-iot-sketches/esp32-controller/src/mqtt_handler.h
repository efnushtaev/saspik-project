#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <stdint.h>

// Объявление функций
void controlRelay(int relayPin, const char* relayName, const char* state);
void publishRelayState(const char* topic, const char* state);
void mqttCallback(char *topic, uint8_t *payload, unsigned int length);

#endif