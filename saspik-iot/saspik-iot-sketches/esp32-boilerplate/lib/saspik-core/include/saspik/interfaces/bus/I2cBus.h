// SPDX-License-Identifier: MIT
/*
 * I2cBus.h — обёртка шины I2C (Wire).
 *
 * Значения пинов/частоты приходят из config.h через InterfaceManager —
 * драйверы I2C-устройств получают готовый TwoWire и не знают настроек.
 * Доступ к TwoWire нужен, потому что большинство библиотек датчиков
 * (Adafruit, SparkFun) работают напрямую с Wire API.
 */

#pragma once

#include <stdint.h>

#include "interfaces/bus/IBus.h"

#ifdef ARDUINO
#include <Wire.h>
#endif

/**
 * @brief Шина I2C.
 */
class I2cBus : public IBus {
public:
    /**
     * @param sdaPin  пин SDA (PIN_I2C_SDA)
     * @param sclPin  пин SCL (PIN_I2C_SCL)
     * @param freqHz  частота шины (I2C_FREQ_HZ)
     */
    I2cBus(uint8_t sdaPin, uint8_t sclPin, uint32_t freqHz);

    bool begin() override;
    bool isReady() const override;

#ifdef ARDUINO
    /** @brief Хэндл TwoWire для драйверов устройств. */
    TwoWire& wire();
#endif

private:
    uint8_t m_sdaPin;
    uint8_t m_sclPin;
    uint32_t m_freqHz;
    bool m_ready;

#ifdef ARDUINO
    TwoWire* m_wire;  // глобальный Wire (второй контроллер I2C — TODO)
#endif
};