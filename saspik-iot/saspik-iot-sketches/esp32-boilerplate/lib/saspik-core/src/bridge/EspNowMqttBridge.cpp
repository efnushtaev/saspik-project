// SPDX-License-Identifier: MIT
/*
 * EspNowMqttBridge.cpp — реализация моста ESP-NOW ↔ MQTT.
 */

#include "bridge/EspNowMqttBridge.h"

#include <ArduinoJson.h>
#include <string.h>

#include "espnow/EspNowCodec.h"
#include "net/TopicBuilder.h"

namespace {
// JSON лога/команды помещается в служебное поле ESP-NOW: от лимита пакета
// отнимаем заголовок (type+ts) и рамку поля (id+len+crc).
constexpr size_t ESP_NOW_PAYLOAD_MAX = ESP_NOW_PACKET_MAX - 6;
}  // namespace

EspNowMqttBridge::EspNowMqttBridge(IMqttClient* mqtt, IEspNowClient* espNow,
                                   ILogger* logger, const DeviceInfo& info)
    : m_mqtt(mqtt), m_espNow(espNow), m_logger(logger), m_info(info) {}

void EspNowMqttBridge::begin() {
    // Без регистрации пира esp_now_send вернёт ошибку «peer not found».
    // NODE_TABLE статична (v1); runtime-управление списком — TODO.
    for (size_t i = 0; i < NODE_TABLE_SIZE; ++i) {
        m_espNow->addPeer(NODE_TABLE[i].mac);
    }
}

void EspNowMqttBridge::onEspNowPacket(const uint8_t* srcMac,
                                      const EspNowPacket& packet) {
    switch (packet.type) {
        case ESP_NOW_TYPE_DATA:
            forwardData(srcMac, packet);
            break;
        case ESP_NOW_TYPE_LOG:
            forwardLog(srcMac, packet);
            break;
        default:
            // COMMAND/ACK узлы шлюзу в v1 не шлют — игнорируем молча.
            break;
    }
}

void EspNowMqttBridge::forwardData(const uint8_t* srcMac,
                                   const EspNowPacket& packet) {
    if (m_mqtt == nullptr) {
        return;
    }
    // objectId узла определяем по его MAC — без этого публиковать некуда;
    // этот же фильтр отсекает чужие/случайные пакеты из эфира.
    const NodeEntry* node = findNodeByMac(srcMac);
    if (node == nullptr) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "unknown source mac");
        }
        return;
    }
    StaticJsonDocument<MQTT_PAYLOAD_MAX> doc;
    for (uint8_t i = 0; i < packet.fieldCount; ++i) {
        const EspNowField& field = packet.fields[i];
        const FieldEntry* entry = findField(field.id);
        // v1: поддерживаем только float (FIELD_ID_*); иное — пропуск.
        if (entry == nullptr || field.len != sizeof(float)) {
            continue;
        }
        float value;
        memcpy(&value, field.data, sizeof(value));
        doc[entry->key] = value;
    }
    if (doc.isNull()) {
        return;  // ни одного известного поля — нечего публиковать
    }
    char topic[MQTT_TOPIC_MAX_LEN];
    char payload[MQTT_PAYLOAD_MAX];
    TopicBuilder::dataTopic(topic, sizeof(topic), m_info.unitId(),
                            node->objectId);
    serializeJson(doc, payload, sizeof(payload));
    m_mqtt->publish(topic, payload, false);
}

void EspNowMqttBridge::forwardLog(const uint8_t* srcMac,
                                  const EspNowPacket& packet) {
    if (m_mqtt == nullptr) {
        return;
    }
    const NodeEntry* node = findNodeByMac(srcMac);
    if (node == nullptr) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "log from unknown mac");
        }
        return;
    }
    for (uint8_t i = 0; i < packet.fieldCount; ++i) {
        const EspNowField& field = packet.fields[i];
        if (field.id != ESP_NOW_FIELD_LOG_JSON || field.len == 0) {
            continue;
        }
        char topic[MQTT_TOPIC_MAX_LEN];
        TopicBuilder::logTopic(topic, sizeof(topic), m_info.unitId(),
                               node->objectId);
        // Текст конверта в кадре не null-terminated — копируем с ограничением.
        char payload[LOG_ENVELOPE_MAX_LEN];
        const size_t copyLen = field.len < sizeof(payload) - 1
                                   ? field.len
                                   : sizeof(payload) - 1;
        memcpy(payload, field.data, copyLen);
        payload[copyLen] = '\0';
        m_mqtt->publish(topic, payload, false);
        return;
    }
}

void EspNowMqttBridge::onMessage(const char* topic, const char* payload) {
    handleCommand(topic, payload);
}

void EspNowMqttBridge::handleCommand(const char* topic, const char* payload) {
    if (m_espNow == nullptr || payload == nullptr) {
        return;
    }
    // Топик device/{unitId}/{objectId}: objectId узла — последний сегмент.
    char nodeObjectId[DEVICE_INFO_OBJECT_LEN];
    nodeObjectIdFromTopic(nodeObjectId, sizeof(nodeObjectId), topic);
    // Команда на сам шлюз (свои исполнители) — не узел: её обрабатывает
    // GatewayApp. Страховка на случай изменения порядка подписок.
    if (strcmp(nodeObjectId, m_info.objectId()) == 0) {
        return;
    }
    const NodeEntry* node = findNodeByObjectId(nodeObjectId);
    if (node == nullptr) {
        if (m_logger != nullptr) {
            m_logger->logWarn("mqtt", "command to unknown node");
        }
        return;
    }
    // Прозрачная упаковка: JSON узлу не разбираем (SRP — узел сам знает
    // свои команды, шлюз — маршрутизатор).
    size_t jsonLen = 0;
    while (jsonLen < ESP_NOW_PAYLOAD_MAX && payload[jsonLen] != '\0') {
        ++jsonLen;
    }
    if (jsonLen == 0) {
        return;
    }
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_COMMAND;
    packet.timestampMs = 0;  // время источника команде не нужно
    EspNowField& field = packet.fields[0];
    field.id = ESP_NOW_FIELD_COMMAND_JSON;
    field.data = reinterpret_cast<const uint8_t*>(payload);
    field.len = static_cast<uint8_t>(jsonLen);
    packet.fieldCount = 1;

    uint8_t buffer[ESP_NOW_PACKET_MAX];
    const size_t len = EspNowCodec::encode(packet, buffer, sizeof(buffer));
    if (len == 0) {
        return;
    }
    m_espNow->send(node->mac, buffer, len);
}

const NodeEntry* EspNowMqttBridge::findNodeByMac(const uint8_t* mac) const {
    for (size_t i = 0; i < NODE_TABLE_SIZE; ++i) {
        if (memcmp(NODE_TABLE[i].mac, mac, 6) == 0) {
            return &NODE_TABLE[i];
        }
    }
    return nullptr;
}

const NodeEntry* EspNowMqttBridge::findNodeByObjectId(
    const char* objectId) const {
    for (size_t i = 0; i < NODE_TABLE_SIZE; ++i) {
        if (strcmp(NODE_TABLE[i].objectId, objectId) == 0) {
            return &NODE_TABLE[i];
        }
    }
    return nullptr;
}

const FieldEntry* EspNowMqttBridge::findField(uint8_t id) const {
    for (size_t i = 0; i < FIELD_DICTIONARY_SIZE; ++i) {
        if (FIELD_DICTIONARY[i].id == id) {
            return &FIELD_DICTIONARY[i];
        }
    }
    return nullptr;
}

void EspNowMqttBridge::nodeObjectIdFromTopic(char* out, size_t cap,
                                             const char* topic) const {
    const char* lastSlash = strrchr(topic, '/');
    const char* objectId = (lastSlash != nullptr) ? lastSlash + 1 : topic;
    const size_t len = strlen(objectId);
    const size_t copyLen = len < cap - 1 ? len : cap - 1;
    memcpy(out, objectId, copyLen);
    out[copyLen] = '\0';
}