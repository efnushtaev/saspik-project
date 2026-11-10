// SPDX-License-Identifier: MIT
/*
 * main.cpp — точка входа прошивки шлюза unitId2.
 *
 * Устройство собрано на шаблоне GatewayApp из esp32-boilerplate: WiFi + MQTT +
 * ESP-NOW-мост + собственные каналы (DHT22 + LED). Здесь только создание
 * единственного IApp и неблокирующий loop с yield() (урок esp32-local-mqtt:
 * без yield() длительные операции приводят к Task WDT / rst:0x8).
 *
 * Файл обёрнут в #ifdef ARDUINO: при host-сборке (native) он пуст.
 */

#ifdef ARDUINO

#include <Arduino.h>

#include "app/GatewayApp.h"
#include "app/IApp.h"
#include "config.h"

namespace {

// Единственный экземпляр приложения живёт между setup() и loop().
IApp* g_app = nullptr;

}  // namespace

void setup() {
    Serial.begin(SERIAL_BAUD);
    g_app = new GatewayApp();
    if (g_app != nullptr) {
        g_app->begin();
    }
}

void loop() {
    // Кормим Task WDT и даём время фоновым задачам WiFi/стека.
    yield();
    if (g_app != nullptr) {
        g_app->update();
    }
}

#endif  // ARDUINO
