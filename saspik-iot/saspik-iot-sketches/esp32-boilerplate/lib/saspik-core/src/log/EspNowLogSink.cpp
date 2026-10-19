// SPDX-License-Identifier: MIT
/*
 * EspNowLogSink.cpp — упаковка конверта в ESP-NOW-пакет.
 *
 * Чистый модуль: зависит от IEspNowClient/EspNowCodec, компилируется на хосте.
 * Ограничение: конверт должен помещаться в лимит ESP-NOW (250 байт);
 * превышение молча отбрасывается send() — TODO: разбиение/сжатие логов.
 */

#include "log/EspNowLogSink.h"

#include <string.h>

#include "espnow/EspNowCodec.h"

EspNowLogSink::EspNowLogSink(IEspNowClient& espNow, const uint8_t* gatewayMac)
    : m_espNow(espNow) {
    memcpy(m_gatewayMac, gatewayMac, sizeof(m_gatewayMac));
}

void EspNowLogSink::write(const LogRecord& /*record*/, const char* jsonEnvelope) {
    const size_t len = strlen(jsonEnvelope);
    // Лимит поля len=1 байт и общий лимит пакета 250: header(5)+crc(1) = 6.
    if (len == 0 || len > ESP_NOW_PACKET_MAX - 6 || len > 255) {
        return;
    }

    EspNowField field;
    field.id = ESP_NOW_FIELD_LOG_JSON;
    field.data = reinterpret_cast<const uint8_t*>(jsonEnvelope);
    field.len = static_cast<uint8_t>(len);

    // Метка времени берётся из uptime сенсора: точная синхронизация
    // времени узлов — TODO (SNTP/time sync, см. TODO.md).
    EspNowPacket pkt;
    pkt.type = ESP_NOW_TYPE_LOG;
    pkt.timestampMs = 0;
    pkt.fields[0] = field;
    pkt.fieldCount = 1;

    // IEspNowClient работает с сырыми байтами — фрейминг делает кодек
    // (SRP: транспорт не знает формат пакета, см. IEspNowClient.h).
    uint8_t buffer[ESP_NOW_PACKET_MAX];
    const size_t encoded = EspNowCodec::encode(pkt, buffer, sizeof(buffer));
    if (encoded > 0) {
        m_espNow.send(m_gatewayMac, buffer, encoded);
    }
}