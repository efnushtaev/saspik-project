// SPDX-License-Identifier: MIT
/*
 * SpiBus.h — обёртка шины SPI.
 *
 * Пины SCK/MISO/MOSI/CS задаются в config.h (PIN_SPI_*); ESP32 SPI
 * допускает произвольную распиновку, дефолты вариантов плат не используются.
 * CS удерживается неактивным (HIGH) — драйвер устройства сам дёргает его
 * вокруг транзакций (стандартный паттерн Adafruit/SparkFun).
 */

#pragma once

#include <stdint.h>

#include "interfaces/bus/IBus.h"

#ifdef ARDUINO
#include <SPI.h>
#endif

/**
 * @brief Шина SPI.
 */
class SpiBus : public IBus {
public:
    /**
     * @param sckPin  пин SCK (PIN_SPI_SCK)
     * @param misoPin пин MISO (PIN_SPI_MISO)
     * @param mosiPin пин MOSI (PIN_SPI_MOSI)
     * @param csPin   пин CS (PIN_SPI_CS)
     */
    SpiBus(uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin, uint8_t csPin);

    bool begin() override;
    bool isReady() const override;

#ifdef ARDUINO
    /** @brief Хэндл SPIClass для драйверов устройств. */
    SPIClass& spi();

    /** @brief Пин CS — драйвер управляет выбором устройства сам. */
    uint8_t csPin() const;
#endif

private:
    uint8_t m_sckPin;
    uint8_t m_misoPin;
    uint8_t m_mosiPin;
    uint8_t m_csPin;
    bool m_ready;

#ifdef ARDUINO
    SPIClass* m_spi;  // глобальный SPI (VSPI; HSPI — TODO при конфликте)
#endif
};