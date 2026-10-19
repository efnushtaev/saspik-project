// SPDX-License-Identifier: MIT
/*
 * RelayActuator.cpp — реализация примера реле.
 */

#include "app/examples/RelayActuator.h"

#ifdef ARDUINO

#include <Arduino.h>

RelayActuator::RelayActuator(uint8_t pin) : m_pin(pin), m_level(0.0f) {}

bool RelayActuator::begin() {
    pinMode(m_pin, OUTPUT);
    digitalWrite(m_pin, LOW);  // стартуем в выключенном состоянии
    m_level = 0.0f;
    return true;
}

bool RelayActuator::set(float level) {
    // Обрезаем в 0..1: контракт IActuator обещает, что значения за пределами
    // не ломают исполнитель (реле не понимает 2.5).
    m_level = (level < 0.0f) ? 0.0f : (level > 1.0f) ? 1.0f : level;
    digitalWrite(m_pin, m_level >= 0.5f ? HIGH : LOW);
    return true;
}

float RelayActuator::get() const { return m_level; }

#endif  // ARDUINO