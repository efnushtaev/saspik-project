// SPDX-License-Identifier: MIT
/*
 * MockLogSink.h — мок ILogSink для host-тестов.
 *
 * Записывает последний конверт и счётчик записей — проверяет, что Logger
 * рассылает записи по всем sink'ам (сам Logger на хосте не собирается,
 * мок пригодится будущим тестам приложений).
 */

#pragma once

#include <string.h>

#include "log/ILogSink.h"

class MockLogSink : public ILogSink {
public:
    void write(const LogRecord& record, const char* jsonEnvelope) override {
        m_lastLevel = record.level;
        m_lastEvent = record.event;
        m_lastMsg = record.msg;
        strncpy(m_lastEnvelope, jsonEnvelope, sizeof(m_lastEnvelope) - 1);
        m_lastEnvelope[sizeof(m_lastEnvelope) - 1] = '\0';
        ++m_writeCount;
    }

    // ---- Хелперы для тестов ----
    uint32_t writeCount() const { return m_writeCount; }
    const char* lastEnvelope() const { return m_lastEnvelope; }
    const char* lastEvent() const { return m_lastEvent; }
    const char* lastLevel() const { return m_lastLevel; }

private:
    uint32_t m_writeCount = 0;
    const char* m_lastLevel = nullptr;
    const char* m_lastEvent = nullptr;
    const char* m_lastMsg = nullptr;
    char m_lastEnvelope[256] = {};
};