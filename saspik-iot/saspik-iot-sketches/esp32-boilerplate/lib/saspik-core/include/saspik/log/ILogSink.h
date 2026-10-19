// SPDX-License-Identifier: MIT
/*
 * ILogSink.h — контракт потребителя лог-записи.
 *
 * Почему отдельный интерфейс: Logger формирует ЕДИНЫЙ JSON-конверт, а куда он
 * уходит (UART, MQTT, ESP-NOW) — не его дело. Sink'и добавляются в Logger
 * через DI, что позволяет в одном устройстве иметь несколько каналов лога.
 */

#pragma once

#include <stdint.h>

/** @brief Уровень лога. */
enum class LogLevel { kInfo, kWarn, kError };

/**
 * @brief Готовая запись лога: уровень уже строкой ("info"/"warn"/"error").
 */
struct LogRecord {
    const char* level;    ///< Строковый уровень (sink'ам не нужно знать enum)
    const char* event;    ///< Короткое имя события (connect, reboot, diag...)
    const char* msg;      ///< Человекочитаемый текст
    uint32_t uptimeSec;   ///< Время работы устройства на момент записи
};

/**
 * @brief Приёмник лог-записей (SRP: только вывод/передача).
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;

    /**
     * @brief Запись лога.
     * @param record       структурированные поля (для UART-вывода)
     * @param jsonEnvelope готовый JSON-конверт (для MQTT/ESP-NOW), не null
     */
    virtual void write(const LogRecord& record, const char* jsonEnvelope) = 0;

    /**
     * @brief Неблокирующий тик: например, слив очереди при восстановлении связи.
     */
    virtual void update() {}

    /**
     * @brief Вытолкнуть накопленное (вызывается перед сном/ребутом).
     */
    virtual void flush() {}
};