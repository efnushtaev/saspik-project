// SPDX-License-Identifier: MIT
/*
 * LightSleepManager.h — реализация ISleepManager на light sleep.
 *
 * Почему light sleep, а не deep: после light sleep продолжает идти millis()
 * и живёт RAM — не нужно пересобирать Composition Root заново, WiFi/ESP-NOW
 * восстанавливаются быстрее, стек глубже сохраняется. Deep sleep выгоднее
 * по энергии, но ломает цикл работы приложения (TODO: DeepSleepManager,
 * см. TODO.md).
 *
 * Заголовок чистый (без Arduino), реализация в .cpp под #ifdef ARDUINO.
 */

#pragma once

#include <stdint.h>

#include "log/ILogger.h"
#include "sys/ISleepManager.h"

/**
 * @brief Цикл «спим интервал — просыпаемся — работаем» на light sleep.
 */
class LightSleepManager : public ISleepManager {
public:
    /**
     * @param logger опциональный логгер (nullptr разрешён)
     */
    explicit LightSleepManager(ILogger* logger);

    void configure(uint32_t sleepIntervalMs) override;
    bool isSleepDue(uint32_t nowMs) const override;
    void enterSleep() override;
    void setBlocked(bool blocked) override;

private:
    ILogger* m_logger;
    uint32_t m_sleepIntervalMs;
    uint32_t m_wakeSinceMs;  ///< millis() последнего пробуждения (начало цикла)
    bool m_configured;
    bool m_blocked;
};