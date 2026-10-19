// SPDX-License-Identifier: MIT
/*
 * I2cBus.cpp — реализация обёртки I2C.
 */

#include "interfaces/bus/I2cBus.h"

#ifdef ARDUINO

I2cBus::I2cBus(uint8_t sdaPin, uint8_t sclPin, uint32_t freqHz)
    : m_sdaPin(sdaPin),
      m_sclPin(sclPin),
      m_freqHz(freqHz),
      m_ready(false),
      m_wire(nullptr) {}

bool I2cBus::begin() {
    if (m_ready) {
        return true;
    }
    m_wire = &Wire;
    // ESP32-вариант begin принимает пины и частоту явно — никаких
    // дефолтных пинов из вариантов плат.
    m_wire->begin(m_sdaPin, m_sclPin, m_freqHz);
    m_ready = true;
    return true;
}

bool I2cBus::isReady() const { return m_ready; }

TwoWire& I2cBus::wire() { return *m_wire; }

#endif  // ARDUINO