// SPDX-License-Identifier: MIT
/*
 * Logger.h — формирование единого JSON-конверта лога и рассылка по sink'ам.
 *
 * Формат конверта совместим с кластером (telegraf → InfluxDB, бакет logs):
 *   {level, src:"device", event, msg, topic, unitId, objectId, uptime[, cause]}
 * Logger не знает, кто потребляет записи: он один раз собирает конверт и
 * отдаёт его каждому ILogSink (UART читает поля, MQTT публикует JSON).
 */

#pragma once

#include <stddef.h>

#include "config.h"
#include "core/DeviceInfo.h"
#include "log/ILogSink.h"
#include "log/ILogger.h"
#include "log/IRebootCauseStore.h"

/**
 * @brief Центральный логгер: конверт + роутинг по ILogSink.
 */
class Logger : public ILogger {
public:
    /**
     * @param info        идентичность устройства (unitId/objectId)
     * @param rebootCause хранилище причины ребута (для startup-конверта)
     */
    Logger(const DeviceInfo& info, IRebootCauseStore& rebootCause);

    /** @brief Подключить приёмник (до LOG_SINKS_MAX штук). */
    void addSink(ILogSink* sink);

    // ---- ILogger ----
    void logInfo(const char* event, const char* msg) override;
    void logWarn(const char* event, const char* msg) override;
    void logError(const char* event, const char* msg) override;

    /**
     * @brief Стартовый конверт с причиной предыдущего ребута.
     * Вызывается один раз после первого успешного подключения к MQTT
     * (поведение publishStartupLog из esp32-local-mqtt).
     */
    void publishStartup();

    /** @brief Тик всех sink'ов (слив очередей). */
    void update();

    /** @brief Flush всех sink'ов (перед сном). */
    void flush();

private:
    void log(LogLevel level, const char* event, const char* msg);
    void sendToSinks(const char* level, const char* event, const char* msg,
                     const char* cause);

    const DeviceInfo& m_info;
    IRebootCauseStore& m_rebootCause;
    ILogSink* m_sinks[LOG_SINKS_MAX];
    size_t m_sinkCount;
};