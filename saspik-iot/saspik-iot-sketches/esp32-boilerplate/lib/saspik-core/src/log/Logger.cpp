// SPDX-License-Identifier: MIT
/*
 * Logger.cpp — реализация формирования конверта и рассылки.
 *
 * Тело обёрнуто в #ifdef ARDUINO: используется millis() для uptime,
 * поэтому на хосте (env:native) компилируется как пустая единица трансляции.
 * JSON собирается ArduinoJson — тем же способом, что и в esp32-local-mqtt.
 */

#include "log/Logger.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <ArduinoJson.h>

#include <string.h>

#include "net/TopicBuilder.h"

namespace {

const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::kInfo:
            return "info";
        case LogLevel::kWarn:
            return "warn";
        case LogLevel::kError:
            return "error";
    }
    return "info";  // недостижимо; страховка для -Wreturn-type
}

}  // namespace

Logger::Logger(const DeviceInfo& info, IRebootCauseStore& rebootCause)
    : m_info(info), m_rebootCause(rebootCause), m_sinkCount(0) {}

void Logger::addSink(ILogSink* sink) {
    if (sink == nullptr || m_sinkCount >= LOG_SINKS_MAX) {
        return;
    }
    m_sinks[m_sinkCount++] = sink;
}

void Logger::logInfo(const char* event, const char* msg) {
    log(LogLevel::kInfo, event, msg);
}

void Logger::logWarn(const char* event, const char* msg) {
    log(LogLevel::kWarn, event, msg);
}

void Logger::logError(const char* event, const char* msg) {
    log(LogLevel::kError, event, msg);
}

void Logger::log(LogLevel level, const char* event, const char* msg) {
    sendToSinks(levelToString(level), event, msg, nullptr);
}

void Logger::sendToSinks(const char* level, const char* event, const char* msg,
                         const char* cause) {
    char logTopic[64];
    TopicBuilder::logTopic(logTopic, sizeof(logTopic), m_info.unitId(),
                           m_info.objectId());

    StaticJsonDocument<LOG_ENVELOPE_MAX_LEN> doc;
    doc["level"] = level;
    doc["src"] = "device";
    doc["event"] = event;
    doc["msg"] = msg;
    doc["topic"] = logTopic;
    doc["unitId"] = m_info.unitId();
    doc["objectId"] = m_info.objectId();
    doc["uptime"] = static_cast<unsigned long>(millis() / 1000);
    if (cause != nullptr) {
        doc["cause"] = cause;  // поле только в startup-конверте
    }

    char envelope[LOG_ENVELOPE_MAX_LEN];
    serializeJson(doc, envelope, sizeof(envelope));

    const LogRecord record{level, event, msg,
                           static_cast<uint32_t>(millis() / 1000)};
    for (size_t i = 0; i < m_sinkCount; ++i) {
        m_sinks[i]->write(record, envelope);
    }
}

void Logger::publishStartup() {
    if (!m_rebootCause.hasRebootCause()) {
        return;
    }
    sendToSinks("info", "startup", "startup after reboot",
                m_rebootCause.rebootCause());
}

void Logger::update() {
    for (size_t i = 0; i < m_sinkCount; ++i) {
        m_sinks[i]->update();
    }
}

void Logger::flush() {
    for (size_t i = 0; i < m_sinkCount; ++i) {
        m_sinks[i]->flush();
    }
}

#endif  // ARDUINO