// SPDX-License-Identifier: MIT
/*
 * test_esp_now_codec.cpp — host-тесты EspNowCodec.
 *
 * Формат кадра: [type(1)][timestamp LE(4)][fieldId(1)+len(1)+data]*[crc8(1)].
 * Known-answer тест CRC-8/MAXIM: контрольная сумма "123456789" = 0xA1
 * (эталон из каталога CRC, полином 0x31 отражённый → 0x8C, init 0x00).
 */

#include <string.h>

#include <unity.h>

#include "espnow/EspNowCodec.h"

void setUp() {}
void tearDown() {}

void test_crc8_maxim_known_answer() {
    // Эталонная строка из каталога CRC: crc8("123456789") == 0xA1.
    const uint8_t data[] = "123456789";
    TEST_ASSERT_EQUAL_UINT8(0xA1, EspNowCodec::crc8(data, 9));
}

void test_crc8_empty() {
    // CRC пустой последовательности = init = 0x00.
    TEST_ASSERT_EQUAL_UINT8(0x00, EspNowCodec::crc8(nullptr, 0));
}

void test_encode_decode_roundtrip() {
    float temperature = 24.5f;
    float humidity = 65.0f;

    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 0x12345678UL;  // проверка little-endian раскладки
    packet.fields[0].id = FIELD_ID_TEMPERATURE;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(&temperature);
    packet.fields[0].len = sizeof(temperature);
    packet.fields[1].id = FIELD_ID_HUMIDITY;
    packet.fields[1].data = reinterpret_cast<const uint8_t*>(&humidity);
    packet.fields[1].len = sizeof(humidity);
    packet.fieldCount = 2;

    uint8_t buffer[ESP_NOW_PACKET_MAX];
    const size_t len = EspNowCodec::encode(packet, buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(len > 0);

    // Заголовок: type + timestamp LE (78 56 34 12).
    TEST_ASSERT_EQUAL_UINT8(ESP_NOW_TYPE_DATA, buffer[0]);
    TEST_ASSERT_EQUAL_UINT8(0x78, buffer[1]);
    TEST_ASSERT_EQUAL_UINT8(0x56, buffer[2]);
    TEST_ASSERT_EQUAL_UINT8(0x34, buffer[3]);
    TEST_ASSERT_EQUAL_UINT8(0x12, buffer[4]);

    EspNowPacket decoded = {};
    TEST_ASSERT_TRUE(EspNowCodec::decode(buffer, len, decoded));
    TEST_ASSERT_EQUAL_UINT8(ESP_NOW_TYPE_DATA, decoded.type);
    TEST_ASSERT_EQUAL_UINT32(0x12345678UL, decoded.timestampMs);
    TEST_ASSERT_EQUAL_UINT8(2, decoded.fieldCount);
    TEST_ASSERT_EQUAL_UINT8(FIELD_ID_TEMPERATURE, decoded.fields[0].id);
    TEST_ASSERT_EQUAL_UINT8(sizeof(float), decoded.fields[0].len);
    float decodedTemp;
    memcpy(&decodedTemp, decoded.fields[0].data, sizeof(decodedTemp));
    TEST_ASSERT_EQUAL_FLOAT(24.5f, decodedTemp);
    TEST_ASSERT_EQUAL_UINT8(FIELD_ID_HUMIDITY, decoded.fields[1].id);
}

void test_decode_rejects_bad_crc() {
    uint8_t buffer[ESP_NOW_PACKET_MAX];
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 1000;
    packet.fields[0].id = FIELD_ID_FLOAT;
    const float value = 1.0f;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(&value);
    packet.fields[0].len = sizeof(value);
    packet.fieldCount = 1;
    const size_t len = EspNowCodec::encode(packet, buffer, sizeof(buffer));

    buffer[0] ^= 0xFF;  // портим байт — CRC не сойдётся
    EspNowPacket decoded = {};
    TEST_ASSERT_FALSE(EspNowCodec::decode(buffer, len, decoded));
}

void test_decode_rejects_truncated() {
    uint8_t buffer[ESP_NOW_PACKET_MAX];
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 1000;
    packet.fields[0].id = FIELD_ID_FLOAT;
    const float value = 1.0f;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(&value);
    packet.fields[0].len = sizeof(value);
    packet.fieldCount = 1;
    const size_t len = EspNowCodec::encode(packet, buffer, sizeof(buffer));

    // Обрезаем CRC — кадр неполный, decode обязан отказать.
    EspNowPacket decoded = {};
    TEST_ASSERT_FALSE(EspNowCodec::decode(buffer, len - 1, decoded));
}

void test_encode_rejects_empty_packet() {
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 0;
    packet.fieldCount = 0;  // без полей пакет бессмысленен

    uint8_t buffer[ESP_NOW_PACKET_MAX];
    TEST_ASSERT_EQUAL_UINT32(0, EspNowCodec::encode(packet, buffer, sizeof(buffer)));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_crc8_maxim_known_answer);
    RUN_TEST(test_crc8_empty);
    RUN_TEST(test_encode_decode_roundtrip);
    RUN_TEST(test_decode_rejects_bad_crc);
    RUN_TEST(test_decode_rejects_truncated);
    RUN_TEST(test_encode_rejects_empty_packet);
    return UNITY_END();
}