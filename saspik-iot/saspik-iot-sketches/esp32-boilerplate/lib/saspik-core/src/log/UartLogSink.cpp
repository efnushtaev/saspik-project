// SPDX-License-Identifier: MIT
/*
 * UartLogSink.cpp — реализация печати в Serial.
 *
 * Обёрнуто в #ifdef ARDUINO: на хосте нет HardwareSerial.
 */

#include "log/UartLogSink.h"

#ifdef ARDUINO

#include <Arduino.h>

UartLogSink::UartLogSink(HardwareSerial& serial) : m_serial(serial) {}

void UartLogSink::write(const LogRecord& record, const char* /*jsonEnvelope*/) {
    // Формат согласован с монитором: [уровень][событие] текст (аптайм).
    m_serial.printf("[%s][%s] %s (%lus)\n", record.level, record.event,
                    record.msg, static_cast<unsigned long>(record.uptimeSec));
}

#endif  // ARDUINO