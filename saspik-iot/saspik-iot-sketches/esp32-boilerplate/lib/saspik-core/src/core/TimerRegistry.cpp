// SPDX-License-Identifier: MIT
/*
 * TimerRegistry.cpp — реализация неблокирующей периодики.
 */

#include "core/TimerRegistry.h"

TimerRegistry::TimerRegistry(ClockFn clock) : m_clock(clock), m_count(0) {}

bool TimerRegistry::add(uint32_t intervalMs, void (*callback)(void* arg), void* arg) {
    if (m_count >= TIMER_TASKS_MAX || callback == nullptr) {
        return false;
    }
    m_tasks[m_count].intervalMs = intervalMs;
    m_tasks[m_count].lastRunMs = 0;
    m_tasks[m_count].callback = callback;
    m_tasks[m_count].arg = arg;
    ++m_count;
    return true;
}

void TimerRegistry::update() {
    if (m_clock == nullptr) {
        return;
    }
    const uint32_t now = m_clock();
    for (size_t i = 0; i < m_count; ++i) {
        Task& t = m_tasks[i];
        // Вычитание без знака корректно переживает переполнение millis():
        // разница (now - last) всегда «сколько прошло», даже если now < last.
        if (now - t.lastRunMs >= t.intervalMs) {
            t.lastRunMs = now;
            if (t.callback != nullptr) {
                t.callback(t.arg);
            }
        }
    }
}