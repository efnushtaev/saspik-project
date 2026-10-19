// SPDX-License-Identifier: MIT
/*
 * UartLogSink.h — вывод лога в Serial (человекочитаемый формат).
 *
 * Печатает структурированные поля LogRecord, а не JSON: в терминале
 * `pio device monitor` JSON-конверт нечитаем, а «почему» события видно сразу.
 * Заголовок зависит от Arduino (HardwareSerial) и используется только прошивкой.
 */

#pragma once

#ifdef ARDUINO

#include <HardwareSerial.h>

#include "log/ILogSink.h"

/**
 * @brief Вывод лог-записей в UART (SRP: только печать).
 */
class UartLogSink : public ILogSink {
public:
    /**
     * @param serial серийный порт (обычно &Serial)
     */
    explicit UartLogSink(HardwareSerial& serial);

    void write(const LogRecord& record, const char* jsonEnvelope) override;

private:
    HardwareSerial& m_serial;
};

#endif  // ARDUINO