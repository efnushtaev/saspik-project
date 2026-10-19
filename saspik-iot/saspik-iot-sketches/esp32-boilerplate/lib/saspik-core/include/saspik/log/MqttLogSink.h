// SPDX-License-Identifier: MIT
/*
 * MqttLogSink.h — публикация конвертов лога в MQTT (device/{unitId}/{objectId}/log).
 *
 * При обрыве связи конверты складываются в ограниченную кольцевую очередь:
 * память фиксирована, поэтому при переполнении теряются СТАРЫЕ записи, а не
 * новые (свежая диагностика ценнее истории; счётчик потерь — m_dropped).
 * Кольцевой файловый лог на LittleFS намеренно не возвращаем (см. IN_PROGRESS.md).
 */

#pragma once

#include <stdint.h>

#include "config.h"
#include "log/ILogSink.h"
#include "mqtt/IMqttClient.h"

/**
 * @brief Публикация JSON-конвертов лога в MQTT с очередью при offline.
 */
class MqttLogSink : public ILogSink {
public:
    /**
     * @param mqtt     клиент MQTT (зависимость через интерфейс)
     * @param unitId   unitId устройства
     * @param objectId objectId устройства (топик строится внутри)
     */
    MqttLogSink(IMqttClient& mqtt, const char* unitId, const char* objectId);

    void write(const LogRecord& record, const char* jsonEnvelope) override;
    void update() override;
    void flush() override;

    /** @brief Текущее число записей в очереди. */
    uint8_t queuedCount() const { return m_count; }

    /** @brief Число вытесненных старых записей (для диагностики). */
    uint32_t droppedCount() const { return m_dropped; }

private:
    void enqueue(const char* envelope);
    bool dequeue(char* out, size_t cap);
    void drain();

    IMqttClient& m_mqtt;
    char m_topic[64];
    char m_queue[LOG_QUEUE_SIZE][LOG_ENVELOPE_MAX_LEN];
    uint8_t m_head;
    uint8_t m_tail;
    uint8_t m_count;
    uint32_t m_dropped;
};