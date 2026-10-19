// SPDX-License-Identifier: MIT
/*
 * OneWireBus.h — обёртка шины OneWire.
 *
 * Один сигнальный пин (PIN_ONE_WIRE) с внешней подтяжкой 4.7k к 3.3V —
 * это требование самой шины, а не boilerplate (см. config.h).
 * Хэндл OneWire нужен драйверам (DallasTemperature и др.).
 */

#pragma once

#include <stdint.h>

#include "interfaces/bus/IBus.h"

#ifdef ARDUINO
#include <OneWire.h>
#endif

/**
 * @brief Шина OneWire.
 */
class OneWireBus : public IBus {
public:
    /**
     * @param dataPin пин данных (PIN_ONE_WIRE)
     */
    explicit OneWireBus(uint8_t dataPin);

    bool begin() override;
    bool isReady() const override;

#ifdef ARDUINO
    /** @brief Хэндл OneWire для драйверов устройств. */
    OneWire& oneWire();
#endif

private:
    uint8_t m_dataPin;
    bool m_ready;

#ifdef ARDUINO
    OneWire m_oneWire;  // композит по значению: begin(pin) задаёт пин
#endif
};