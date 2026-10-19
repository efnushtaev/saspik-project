// SPDX-License-Identifier: MIT
/*
 * InterfaceManager.cpp — создание шин по флагам config.h.
 */

#include "interfaces/bus/InterfaceManager.h"

#ifdef ARDUINO

InterfaceManager::InterfaceManager()
    : m_i2c(nullptr), m_oneWire(nullptr), m_spi(nullptr), m_uart(nullptr) {}

void InterfaceManager::begin() {
    // Каждая шина создаётся только если включена: выключенная не должна
    // занимать ни RAM под объект, ни пины.
    if (BUS_I2C_ENABLED && m_i2c == nullptr) {
        m_i2c = new I2cBus(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQ_HZ);
        m_i2c->begin();
    }
    if (BUS_ONE_WIRE_ENABLED && m_oneWire == nullptr) {
        m_oneWire = new OneWireBus(PIN_ONE_WIRE);
        m_oneWire->begin();
    }
    if (BUS_SPI_ENABLED && m_spi == nullptr) {
        m_spi = new SpiBus(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_SPI_CS);
        m_spi->begin();
    }
    if (BUS_UART_ENABLED && m_uart == nullptr) {
        m_uart = new UartBus(UART_NUM, UART_BAUD, PIN_UART_TX, PIN_UART_RX);
        m_uart->begin();
    }
}

#endif  // ARDUINO