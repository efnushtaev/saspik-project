#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include <esp_sleep.h>
#include "config.h"

/**
 * @brief Управление глубоким сном ESP32 (SRP)
 * 
 * Отвечает за сохранение времени в RTC-память,
 * настройку таймера пробуждения и переход в глубокий сон.
 */
class SleepManager {
public:
    SleepManager();

    /**
     * @brief Сохранить текущее время и перейти в глубокий сон
     */
    void goToSleep();

    /**
     * @brief Получить накопленное время (rtcMillis)
     */
    unsigned long getRtcMillis() const { return _rtcMillis; }

    /**
     * @brief Сбросить накопленное время (при первом запуске)
     */
    void resetRtcMillis() { _rtcMillis = 0; }

    /**
     * @brief Проверить, было ли пробуждение по таймеру
     * @return true если пробуждение из глубокого сна
     */
    bool isWakeupFromSleep() const;

private:
    RTC_DATA_ATTR static unsigned long _rtcMillis;
};

#endif // SLEEP_MANAGER_H