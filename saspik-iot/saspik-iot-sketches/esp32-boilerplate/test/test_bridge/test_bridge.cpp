// SPDX-License-Identifier: MIT
/*
 * test_bridge.cpp — host-тесты EspNowMqttBridge (маппинг ESP-NOW ↔ MQTT).
 *
 * Проверяются три направления моста:
 *   узел → шлюз: DATA → sensor/{unitId}/{objectIdУзла} (fieldId → ключ JSON);
 *   узел → шлюз: LOG → device/{unitId}/{objectIdУзла}/log (конверт as-is);
 *   шлюз → узел: MQTT-команда → ESP-NOW COMMAND (JSON без разбора).
 *
 * NODE_TABLE (config.h) содержит узел "saspik-senb001" с MAC {0,0,0,0,0,0} —
 * тесты используют этот MAC как источник пакетов.
 */

#include <string.h>

#include <ArduinoJson.h>
#include <unity.h>

#include "MockEspNowClient.h"
#include "MockMqttClient.h"
#include "bridge/EspNowMqttBridge.h"
#include "core/DeviceInfo.h"
#include "espnow/EspNowCodec.h"

namespace {
const uint8_t kNodeMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t kUnknownMac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

MockMqttClient mqtt;
MockEspNowClient espNow;
DeviceInfo info("unitId2", "saspik-gwqm001");
EspNowMqttBridge* bridge = nullptr;
}  // namespace

void setUp() {
    // Моки — глобальные объекты: сбрасываем счётчики между тестами.
    mqtt.reset();
    espNow.reset();
    // Моки стартуют «отключёнными»: publish/send без этого молча ничего
    // не делают (проверяем логику моста, а не состояние соединения).
    mqtt.setConnected(true);
    espNow.begin(ESP_NOW_CHANNEL);
    bridge = new EspNowMqttBridge(&mqtt, &espNow, nullptr, info);
}

void tearDown() {
    delete bridge;
    bridge = nullptr;
}

void test_data_packet_published_to_sensor_topic() {
    float temperature = 24.5f;
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 1000;
    packet.fields[0].id = FIELD_ID_TEMPERATURE;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(&temperature);
    packet.fields[0].len = sizeof(temperature);
    packet.fieldCount = 1;

    bridge->onEspNowPacket(kNodeMac, packet);

    TEST_ASSERT_EQUAL_STRING("sensor/unitId2/saspik-senb001",
                             mqtt.lastTopic());
    StaticJsonDocument<128> doc;
    TEST_ASSERT_FALSE(deserializeJson(doc, mqtt.lastPayload()));
    TEST_ASSERT_EQUAL_FLOAT(24.5f, doc["temperature"].as<float>());
}

void test_log_packet_published_as_is() {
    const char* envelope = "{\"level\":\"info\",\"event\":\"test\"}";
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_LOG;
    packet.timestampMs = 1000;
    packet.fields[0].id = ESP_NOW_FIELD_LOG_JSON;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(envelope);
    packet.fields[0].len = static_cast<uint8_t>(strlen(envelope));
    packet.fieldCount = 1;

    bridge->onEspNowPacket(kNodeMac, packet);

    TEST_ASSERT_EQUAL_STRING("device/unitId2/saspik-senb001/log",
                             mqtt.lastTopic());
    TEST_ASSERT_EQUAL_STRING(envelope, mqtt.lastPayload());
}

void test_unknown_source_mac_is_ignored() {
    float temperature = 24.5f;
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = 1000;
    packet.fields[0].id = FIELD_ID_TEMPERATURE;
    packet.fields[0].data = reinterpret_cast<const uint8_t*>(&temperature);
    packet.fields[0].len = sizeof(temperature);
    packet.fieldCount = 1;

    bridge->onEspNowPacket(kUnknownMac, packet);

    // Чужой MAC не публикуем: objectId узла определяется по таблице NODE_TABLE.
    TEST_ASSERT_EQUAL_UINT32(0, mqtt.publishCount());
}

void test_mqtt_command_forwarded_as_esp_now_command() {
    const char* command = "{\"state\":\"ON\"}";
    bridge->onMessage("device/unitId2/saspik-senb001", command);

    TEST_ASSERT_EQUAL_UINT32(1, espNow.sendCount());
    TEST_ASSERT_EQUAL_UINT8_ARRAY(kNodeMac, espNow.lastMac(), 6);

    // Шлюз — маршрутизатор: JSON узлу передаётся без разбора (см. заголовок).
    EspNowPacket decoded = {};
    TEST_ASSERT_TRUE(
        EspNowCodec::decode(espNow.lastData(), espNow.lastLen(), decoded));
    TEST_ASSERT_EQUAL_UINT8(ESP_NOW_TYPE_COMMAND, decoded.type);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.fieldCount);
    TEST_ASSERT_EQUAL_UINT8(ESP_NOW_FIELD_COMMAND_JSON, decoded.fields[0].id);
    char payload[64];
    memcpy(payload, decoded.fields[0].data, decoded.fields[0].len);
    payload[decoded.fields[0].len] = '\0';
    TEST_ASSERT_EQUAL_STRING(command, payload);
}

void test_command_to_unknown_node_is_ignored() {
    bridge->onMessage("device/unitId2/saspik-unknown", "{\"state\":\"ON\"}");
    TEST_ASSERT_EQUAL_UINT32(0, espNow.sendCount());
}

void test_command_to_gateway_itself_is_ignored() {
    // Свои команды шлюз обрабатывает сам (GatewayApp), мост их не трогает.
    bridge->onMessage("device/unitId2/saspik-gwqm001", "{\"state\":\"ON\"}");
    TEST_ASSERT_EQUAL_UINT32(0, espNow.sendCount());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_data_packet_published_to_sensor_topic);
    RUN_TEST(test_log_packet_published_as_is);
    RUN_TEST(test_unknown_source_mac_is_ignored);
    RUN_TEST(test_mqtt_command_forwarded_as_esp_now_command);
    RUN_TEST(test_command_to_unknown_node_is_ignored);
    RUN_TEST(test_command_to_gateway_itself_is_ignored);
    return UNITY_END();
}