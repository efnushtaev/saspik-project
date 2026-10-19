// SPDX-License-Identifier: MIT
/*
 * ILogger.h — узкий контракт «умею писать лог».
 *
 * Вводится ради слабой связанности: менеджеры (WifiManager, MqttManager,
 * EspNowMqttBridge) хотят сообщать о событиях, но не должны знать про
 * устройство JSON-конвертов. Nullable-указатель ILogger* позволяет и вовсе
 * работать без лога (например, в тестах или при выключенном логировании).
 */

#pragma once

/**
 * @brief Минимальный интерфейс логирования событий.
 */
class ILogger {
public:
    virtual ~ILogger() = default;

    /** @brief Информационное событие. */
    virtual void logInfo(const char* event, const char* msg) = 0;

    /** @brief Предупреждение (не фатально, но заметно). */
    virtual void logWarn(const char* event, const char* msg) = 0;

    /** @brief Ошибка. */
    virtual void logError(const char* event, const char* msg) = 0;
};