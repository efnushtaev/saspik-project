// SPDX-License-Identifier: MIT
/*
 * RelayActuator.h — пример IActuator: реле на GPIO.
 *
 * Уровень 0..1 (контракт IActuator): реле трактует как on/off (>= 0.5 — вкл),
 * диммер — как скважность ШИМ. Единая семантика позволяет обрабатывать
 * команды одинаково для любых исполнителей (OCP). Пин приходит в конструкторе
 * (PIN_GPIO_5, см. config.h).
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include <stdint.h>

#include "config.h"
#include "interfaces/actuator/IActuator.h"

/**
 * @brief Пример исполнителя: реле на цифровом выходе.
 */
class RelayActuator : public IActuator {
public:
    /**
     * @param pin пин реле (PIN_GPIO_5, см. config.h)
     */
    explicit RelayActuator(uint8_t pin);

    bool begin() override;
    bool set(float level) override;
    float get() const override;

private:
    uint8_t m_pin;
    float m_level;  ///< текущий уровень 0..1
};

#endif  // ARDUINO