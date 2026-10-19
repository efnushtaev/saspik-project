// SPDX-License-Identifier: MIT
/*
 * SpiBus.cpp — реализация обёртки SPI.
 */

#include "interfaces/bus/SpiBus.h"

#ifdef ARDUINO

#include <Arduino.h>

SpiBus::SpiBus(uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin, uint8_t csPin)
    : m_sckPin(sckPin),
      m_misoPin(misoPin),
      m_mosiPin(mosiPin),
      m_csPin(csPin),
      m_ready(false),
      m_spi(nullptr) {}

bool SpiBus::begin() {
    if (m_ready) {
        return true;
    }
    m_spi = &SPI;
    m_spi->begin(m_sckPin, m_misoPin, m_mosiPin, m_csPin);
    // CS неактивен: выбор устройства делает драйвер вокруг транзакций.
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    m_ready = true;
    return true;
}

bool SpiBus::isReady() const { return m_ready; }

SPIClass& SpiBus::spi() { return *m_spi; }

uint8_t SpiBus::csPin() const { return m_csPin; }

#endif  // ARDUINO