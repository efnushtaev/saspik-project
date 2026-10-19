// SPDX-License-Identifier: MIT
/*
 * FloatSensor.h — пример ISensor: поплавковый датчик (геркон).
 *
 * Цифровой вход с подтяжкой (INPUT_PULLUP): замкнутый контакт (вода есть) =
 * LOW = 1.0, разомкнутый = HIGH = 0.0. Пин приходит в конструкторе
 * (PIN_GPIO_4, см. config.h) — датчик не знает, где он распаян.
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include <stdint.h>

#include "config.h"
#include "interfaces/sensor/ISensor.h"

/**
 * @brief Пример датчика уровня воды (геркон на GPIO).
 */
class FloatSensor : public ISensor {
public:
    /**
     * @param pin пин геркона (PIN_GPIO_4, см. config.h)
     */
    explicit FloatSensor(uint8_t pin);

    bool begin() override;
    SensorSample read() override;

private:
    uint8_t m_pin;
};

#endif  // ARDUINO