// SPDX-License-Identifier: MIT
/*
 * TimerRegistry.h — неблокирующая периодика.
 *
 * Почему отдельный класс, а не millis() в каждом модуле: во-первых, единый
 * источник «пора ли тикать» делает loop детерминированным и тестируемым;
 * во-вторых, функция времени инжектируется через конструктор — на хосте
 * тесты подставляют фейковые часы, в прошивке передаётся &millis.
 * Разница времени считается через вычитание без знака (overflow-safe).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"

/**
 * @brief Реестр периодических задач с одним неблокирующим update().
 */
class TimerRegistry {
public:
    /** @brief Сигнатура источника времени (uint32_t, как millis()). */
    using ClockFn = uint32_t (*)();

    /** @brief Описание одной периодической задачи. */
    struct Task {
        uint32_t intervalMs;                 ///< Период выполнения
        uint32_t lastRunMs;                  ///< Момент последнего запуска
        void (*callback)(void* arg) = nullptr;  ///< Обратный вызов
        void* arg = nullptr;                 ///< Контекст вызова (например, this App)
    };

    /**
     * @param clock функция текущего времени в мс (в прошивке — &millis)
     */
    explicit TimerRegistry(ClockFn clock);

    /**
     * @brief Регистрация задачи.
     * @return false, если реестр заполнен (TIMER_TASKS_MAX).
     */
    bool add(uint32_t intervalMs, void (*callback)(void* arg), void* arg);

    /**
     * @brief Вызывает все задачи, чей интервал истёк. Не блокируется.
     */
    void update();

    /** @brief Количество зарегистрированных задач. */
    size_t count() const { return m_count; }

private:
    ClockFn m_clock;
    Task m_tasks[TIMER_TASKS_MAX];
    size_t m_count;
};