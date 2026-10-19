// SPDX-License-Identifier: MIT
/*
 * EspNowCodec.cpp — реализация фрейминга.
 *
 * Раскладка байт (little-endian):
 *   [0]      type
 *   [1..4]   timestampMs
 *   далее    повторяется: [id(1)] [len(1)] [data(len)]
 *   [last]   crc8 по всем предыдущим байтам
 * Любое нарушение (CRC, выход за границы, переполнение полей) — reject:
 * пакет отбрасывается целиком. Повторные передачи (retry/ACK) — TODO.
 */

#include "espnow/EspNowCodec.h"

#include <string.h>

namespace {
constexpr size_t HEADER_LEN = 5;  // type(1) + timestamp(4)
constexpr size_t CRC_LEN = 1;
}  // namespace

size_t EspNowCodec::encode(const EspNowPacket& packet, uint8_t* out,
                           size_t cap) {
    if (out == nullptr || packet.fieldCount == 0 ||
        packet.fieldCount > ESP_NOW_MAX_FIELDS) {
        return 0;  // пустой пакет бессмысленен, а лимит полей — защита таблиц
    }
    size_t pos = 0;
    if (pos + HEADER_LEN + CRC_LEN > cap) {
        return 0;
    }
    out[pos++] = packet.type;
    out[pos++] = static_cast<uint8_t>(packet.timestampMs & 0xFF);
    out[pos++] = static_cast<uint8_t>((packet.timestampMs >> 8) & 0xFF);
    out[pos++] = static_cast<uint8_t>((packet.timestampMs >> 16) & 0xFF);
    out[pos++] = static_cast<uint8_t>((packet.timestampMs >> 24) & 0xFF);

    for (uint8_t i = 0; i < packet.fieldCount; ++i) {
        const EspNowField& field = packet.fields[i];
        if (field.len > 0 && field.data == nullptr) {
            return 0;  // ненулевая длина без данных — ошибка кодирования
        }
        if (pos + 2 + field.len + CRC_LEN > cap) {
            return 0;  // поле не влезает (превышен ESP_NOW_PACKET_MAX)
        }
        out[pos++] = field.id;
        out[pos++] = field.len;
        if (field.len > 0) {
            memcpy(out + pos, field.data, field.len);
            pos += field.len;
        }
    }
    out[pos] = crc8(out, pos);
    return pos + CRC_LEN;
}

bool EspNowCodec::decode(const uint8_t* data, size_t len, EspNowPacket& out) {
    // Минимум: заголовок + CRC (пакет без полей данных).
    if (data == nullptr || len < HEADER_LEN + CRC_LEN) {
        return false;
    }
    if (data[len - 1] != crc8(data, len - 1)) {
        return false;  // повреждённый кадр — отбрасываем целиком
    }
    size_t pos = 0;
    out.type = data[pos++];
    out.timestampMs = static_cast<uint32_t>(data[pos]) |
                      (static_cast<uint32_t>(data[pos + 1]) << 8) |
                      (static_cast<uint32_t>(data[pos + 2]) << 16) |
                      (static_cast<uint32_t>(data[pos + 3]) << 24);
    pos += 4;

    out.fieldCount = 0;
    while (pos + 1 < len - CRC_LEN) {  // последний байт — CRC, не поле
        if (out.fieldCount >= ESP_NOW_MAX_FIELDS) {
            return false;  // защита от переполнения таблицы полей
        }
        EspNowField& field = out.fields[out.fieldCount];
        field.id = data[pos++];
        field.len = data[pos++];
        if (pos + field.len > len - CRC_LEN) {
            return false;  // поле вылезло за пределы полезной части
        }
        field.data = field.len > 0 ? data + pos : nullptr;
        pos += field.len;
        ++out.fieldCount;
    }
    return pos == len - CRC_LEN;  // вся полезная часть разобрана до CRC
}

uint8_t EspNowCodec::crc8(const uint8_t* data, size_t len) {
    // CRC-8/MAXIM (Dallas): init 0x00, отражённый полином 0x31 → 0x8C, xor-out нет.
    uint8_t crc = 0x00;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 0x01U) ? static_cast<uint8_t>((crc >> 1) ^ 0x8C)
                                : static_cast<uint8_t>(crc >> 1);
        }
    }
    return crc;
}