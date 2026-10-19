// SPDX-License-Identifier: MIT
/*
 * MqttLogSink.cpp — реализация очереди и публикации.
 *
 * Чистый модуль: зависит только от IMqttClient и TopicBuilder,
 * поэтому компилируется и на хосте (не используется тестами).
 */

#include "log/MqttLogSink.h"

#include <stdio.h>
#include <string.h>

#include "net/TopicBuilder.h"

MqttLogSink::MqttLogSink(IMqttClient& mqtt, const char* unitId,
                         const char* objectId)
    : m_mqtt(mqtt), m_head(0), m_tail(0), m_count(0), m_dropped(0) {
    TopicBuilder::logTopic(m_topic, sizeof(m_topic), unitId, objectId);
}

void MqttLogSink::write(const LogRecord& /*record*/, const char* jsonEnvelope) {
    if (m_mqtt.isConnected()) {
        m_mqtt.publish(m_topic, jsonEnvelope, false);
        return;
    }
    enqueue(jsonEnvelope);  // связь недоступна — откладываем, не теряем
}

void MqttLogSink::update() { drain(); }

void MqttLogSink::flush() { drain(); }

void MqttLogSink::enqueue(const char* envelope) {
    if (m_count >= LOG_QUEUE_SIZE) {
        // Очередь полна: старейшая запись вытесняется — свежий диагноз важнее.
        dequeue(m_queue[m_head], LOG_ENVELOPE_MAX_LEN);
        ++m_dropped;
    }
    const size_t len = strlen(envelope);
    char* slot = m_queue[m_tail];
    memcpy(slot, envelope, len);
    slot[len] = '\0';
    m_tail = static_cast<uint8_t>((m_tail + 1) % LOG_QUEUE_SIZE);
    ++m_count;
}

bool MqttLogSink::dequeue(char* out, size_t cap) {
    if (m_count == 0) {
        return false;
    }
    snprintf(out, cap, "%s", m_queue[m_head]);
    m_head = static_cast<uint8_t>((m_head + 1) % LOG_QUEUE_SIZE);
    --m_count;
    return true;
}

void MqttLogSink::drain() {
    char envelope[LOG_ENVELOPE_MAX_LEN];
    // Сливаем только пока соединение живо: publish при offline снова встал бы в очередь.
    while (m_count > 0 && m_mqtt.isConnected()) {
        if (dequeue(envelope, sizeof(envelope))) {
            m_mqtt.publish(m_topic, envelope, false);
        }
    }
}