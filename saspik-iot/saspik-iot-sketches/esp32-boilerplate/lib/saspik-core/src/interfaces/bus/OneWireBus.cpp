// SPDX-License-Identifier: MIT
/*
 * OneWireBus.cpp — реализация обёртки OneWire.
 */

#include "interfaces/bus/OneWireBus.h"

#ifdef ARDUINO

OneWireBus::OneWireBus(uint8_t dataPin) : m_dataPin(dataPin), m_ready(false) {}

bool OneWireBus::begin() {
    if (m_ready) {
        return true;
    }
    // Сам OneWire не проверяет наличие устройств — поиск адресов делает
    // драйвер (DS18B20 и др.). begin() лишь настраивает пин и тайминги.
    m_oneWire.begin(m_dataPin);
    m_ready = true;
    return true;
}

bool OneWireBus::isReady() const { return m_ready; }

OneWire& OneWireBus::oneWire() { return m_oneWire; }

#endif  // ARDUINO