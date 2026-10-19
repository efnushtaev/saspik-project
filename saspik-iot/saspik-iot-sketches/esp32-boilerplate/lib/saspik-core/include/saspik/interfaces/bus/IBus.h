// SPDX-License-Identifier: MIT
/*
 * IBus.h — общий контракт аппаратной шины.
 *
 * Шины инициализируются из config.h (включённость + пины), поэтому драйверы
 * устройств не знают ни пинов, ни частот — они получают готовый хэндл шины
 * через InterfaceManager. Единый интерфейс позволяет InterfaceManager
 * управлять любыми шинами одинаково (DIP через полиморфизм).
 */

#pragma once

/**
 * @brief Инициализируемая аппаратная шина.
 */
class IBus {
public:
    virtual ~IBus() = default;

    /**
     * @brief Инициализация шины (пины/частота заданы в конструкторе).
     * @return false, если железо не ответило (шину можно игнорировать)
     */
    virtual bool begin() = 0;

    /** @brief true, если begin() прошёл успешно. */
    virtual bool isReady() const = 0;
};