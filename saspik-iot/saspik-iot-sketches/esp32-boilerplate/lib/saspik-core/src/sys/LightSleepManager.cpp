// SPDX-License-Identifier: MIT
/*
 * LightSleepManager.cpp — реализация на esp_sleep API.
 *
 * Ключевая деталь: после esp_light_sleep_start() millis() ПРОДОЛЖАЕТ идти
 * (в отличие от deep sleep), поэтому точку отсчёта цикла храним явно
 * (m_wakeSinceMs) и перезапускаем её сразу после пробуждения — так
 * isSleepDue() корректно считает интервал «работы + сна» без внешнего
 * состояния.
 */

#include "sys/LightSleepManager.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <esp_sleep.h>

LightSleepManager::LightSleepManager(ILogger* logger)
    : m_logger(logger),
      m_sleepIntervalMs(0),
      m_wakeSinceMs(0),
      m_configured(false),
      m_blocked(false) {}

void LightSleepManager::configure(uint32_t sleepIntervalMs) {
    m_sleepIntervalMs = sleepIntervalMs;
    m_wakeSinceMs = millis();  // старт первого цикла — от начала работы
    m_configured = true;
}

bool LightSleepManager::isSleepDue(uint32_t nowMs) const {
    if (!m_configured || m_blocked || m_sleepIntervalMs == 0) {
        return false;
    }
    return nowMs - m_wakeSinceMs >= m_sleepIntervalMs;
}

void LightSleepManager::enterSleep() {
    if (!m_configured || m_blocked || m_sleepIntervalMs == 0) {
        return;
    }
    if (m_logger != nullptr) {
        m_logger->logInfo("sleep", "entering light sleep");
    }
    // Таймер пробуждения в микросекундах; uint64 — не переполняется
    // даже при суточных интервалах.
    esp_sleep_enable_timer_wakeup(
        static_cast<uint64_t>(m_sleepIntervalMs) * 1000ULL);
    esp_light_sleep_start();
    // Мы здесь только после пробуждения; millis() продолжил идти.
    m_wakeSinceMs = millis();
    if (m_logger != nullptr) {
        m_logger->logInfo("sleep", "woke up");
    }
}

void LightSleepManager::setBlocked(bool blocked) {
    // Пока заблокировано — не спим: портал должен отвечать, очереди логов
    // должны успевать уходить.
    m_blocked = blocked;
}

#endif  // ARDUINO