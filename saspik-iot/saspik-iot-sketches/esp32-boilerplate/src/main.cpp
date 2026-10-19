// SPDX-License-Identifier: MIT
/*
 * main.cpp — точка входа прошивки: выбор шаблона по DEVICE_ROLE.
 *
 * Вся логика живёт в App-классах (Composition Root); здесь только:
 *   - Serial (лог-канал UART);
 *   - создание IApp по build-флагу DEVICE_ROLE (см. config.h);
 *   - неблокирующий loop с yield() (кормление Task WDT — урок esp32-local-mqtt).
 *
 * Файл обёрнут в #ifdef ARDUINO: в env:native (host-тесты) он компилируется
 * как пустая единица трансляции — Arduino-точки входа на хосте не нужны.
 */

#ifdef ARDUINO

#include <Arduino.h>

#include "config.h"
#include "app/DeviceApp.h"
#include "app/GatewayApp.h"
#include "app/IApp.h"
#include "app/SensorApp.h"

namespace {

// Единственный экземпляр приложения живёт между setup() и loop().
// Статический указатель — вынужденная мера Arduino: у точки входа нет
// способа передать состояние из setup в loop иначе. Сам граф объектов
// (и все «глобалы» модулей) — внутри App, наружу ничего не торчит.
IApp* g_app = nullptr;

IApp* createApp() {
    switch (DEVICE_ROLE) {
        case DEVICE_ROLE_GATEWAY:
            return new GatewayApp();
        case DEVICE_ROLE_SENSOR:
            return new SensorApp();
        case DEVICE_ROLE_DEVICE:
            return new DeviceApp();
    }
    return nullptr;  // неизвестная роль — прошивка не стартует (см. config.h)
}

}  // namespace

void setup() {
    Serial.begin(SERIAL_BAUD);
    g_app = createApp();
    if (g_app != nullptr) {
        g_app->begin();
    }
}

void loop() {
    // Кормим Task WDT и даём время фоновым задачам WiFi/стека: без yield
    // длительные операции (connect и т.п.) вызывают rst:0x8 (урок esp32-local-mqtt).
    yield();
    if (g_app != nullptr) {
        g_app->update();
    }
}

#endif  // ARDUINO