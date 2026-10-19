// SPDX-License-Identifier: MIT
/*
 * ISleepManager.h — контракт управления сном (для Sensor).
 *
 * Sensor спит по таймеру (SLEEP_INTERVAL_MS, переопределяется через NVS),
 * просыпается, читает/шлёт данные и засыпает снова. Абстракция нужна, чтобы
 * SensorApp не знал деталей esp_sleep и чтобы в тестах можно было подменить
 * «сон» заглушкой. Блокировка (setBlocked) — на время настройки через портал
 * или других операций, когда засыпать нельзя.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Управление циклом сон/пробуждение.
 */
class ISleepManager {
public:
    virtual ~ISleepManager() = default;

    /**
     * @brief Задать интервал сна (вызывается один раз в setup).
     * @param sleepIntervalMs длительность одного цикла сна
     */
    virtual void configure(uint32_t sleepIntervalMs) = 0;

    /**
     * @brief true, если пора засыпать (прошёл интервал с пробуждения).
     * @param nowMs текущий millis()
     */
    virtual bool isSleepDue(uint32_t nowMs) const = 0;

    /**
     * @brief Уйти в сон (блокирует до пробуждения).
     * Вызывающий обязан предварительно сбросить/вытолкнуть логи (Logger::flush),
     * иначе конверты, накопившиеся в очередях, будут потеряны.
     */
    virtual void enterSleep() = 0;

    /**
     * @brief Временно запретить/разрешить засыпание.
     * @param blocked true — isSleepDue() всегда false (портал, OTA, отладка)
     */
    virtual void setBlocked(bool blocked) = 0;
};