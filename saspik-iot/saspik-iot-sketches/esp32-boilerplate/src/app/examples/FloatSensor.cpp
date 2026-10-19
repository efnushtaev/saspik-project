// SPDX-License-Identifier: MIT
/*
 * FloatSensor.cpp — реализация примера геркона.
 */

#include "app/examples/FloatSensor.h"

#ifdef ARDUINO

#include <Arduino.h>

FloatSensor::FloatSensor(uint8_t pin) : m_pin(pin) {}

bool FloatSensor::begin() {
    // Подтяжка к VCC: замкнутый контакт даёт LOW (вода есть).
    pinMode(m_pin, INPUT_PULLUP);
    return true;
}

SensorSample FloatSensor::read() {
    SensorSample sample = {};
    sample.fieldId = FIELD_ID_FLOAT;
    // LOW = контакт замкнут = вода есть (см. заголовок класса).
    sample.value = (digitalRead(m_pin) == LOW) ? 1.0f : 0.0f;
    sample.valid = true;
    return sample;
}

#endif  // ARDUINO