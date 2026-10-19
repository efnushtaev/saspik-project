// SPDX-License-Identifier: MIT
/*
 * EspNowLogSink.h — передача лог-конвертов узла в шлюз по ESP-NOW.
 *
 * Нужен Sensor/Device шаблонам, у которых нет прямого MQTT: их конверты
 * упаковываются в пакет типа kLog (поле ESP_NOW_FIELD_LOG_JSON) и отправляются
 * шлюзу, а шлюз публикует их в device/{unitId}/{nodeObjectId}/log.
 */

#pragma once

#include <stdint.h>

#include "espnow/EspNowTypes.h"
#include "espnow/IEspNowClient.h"
#include "log/ILogSink.h"

/**
 * @brief Отправка JSON-конверта лога в шлюз по ESP-NOW.
 */
class EspNowLogSink : public ILogSink {
public:
    /**
     * @param espNow      клиент ESP-NOW
     * @param gatewayMac  MAC шлюза (копируется, 6 байт)
     */
    EspNowLogSink(IEspNowClient& espNow, const uint8_t* gatewayMac);

    void write(const LogRecord& record, const char* jsonEnvelope) override;

private:
    IEspNowClient& m_espNow;
    uint8_t m_gatewayMac[6];
};