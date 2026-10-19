// SPDX-License-Identifier: MIT
/*
 * EspNowMqttBridge.h — мост ESP-NOW ↔ MQTT (шаблон Gateway).
 *
 * Направления:
 *   узел → шлюз: данные (ESP_NOW_TYPE_DATA) публикуются в
 *     sensor/{unitId}/{objectIdУзла} (fieldId → имя ключа по FIELD_DICTIONARY);
 *     логи (ESP_NOW_TYPE_LOG) — в device/{unitId}/{objectIdУзла}/log
 *     (JSON-конверт узла передаётся as-is в поле ESP_NOW_FIELD_LOG_JSON);
 *   шлюз → узел: MQTT-команда на device/{unitId}/{objectIdУзла} упаковывается
 *     в ESP-NOW COMMAND с полем ESP_NOW_FIELD_COMMAND_JSON — шлюз работает
 *     маршрутизатором и не разбирает команды (их знает сам узел).
 *
 * Чистый модуль: без Arduino, покрывается host-тестами
 * (MockMqttClient + MockEspNowClient). Словарь полей и таблица узлов —
 * compile-time (config.h); runtime-редактирование — TODO (см. TODO.md).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "core/DeviceInfo.h"
#include "espnow/IEspNowClient.h"
#include "log/ILogger.h"
#include "mqtt/IMqttClient.h"
#include "net/IMessageHandler.h"

/**
 * @brief Прозрачный мост ESP-NOW ↔ MQTT.
 */
class EspNowMqttBridge : public IEspNowListener, public IMessageHandler {
public:
    /**
     * @param mqtt    MQTT-клиент шлюза (публикация данных/логов узлов)
     * @param espNow  ESP-NOW клиент шлюза (отправка команд узлам)
     * @param logger  опциональный логгер (nullptr разрешён)
     * @param info    идентичность самого шлюза (unitId — общий для юнита)
     */
    EspNowMqttBridge(IMqttClient* mqtt, IEspNowClient* espNow, ILogger* logger,
                     const DeviceInfo& info);

    /** @brief Регистрация пиров из NODE_TABLE (один раз, в setup). */
    void begin();

    // ---- IEspNowListener (пакеты от узлов) ----
    void onEspNowPacket(const uint8_t* srcMac,
                        const EspNowPacket& packet) override;

    // ---- IMessageHandler (MQTT-команды узлам) ----
    void onMessage(const char* topic, const char* payload) override;

private:
    void forwardData(const uint8_t* srcMac, const EspNowPacket& packet);
    void forwardLog(const uint8_t* srcMac, const EspNowPacket& packet);
    void handleCommand(const char* topic, const char* payload);
    const NodeEntry* findNodeByMac(const uint8_t* mac) const;
    const NodeEntry* findNodeByObjectId(const char* objectId) const;
    const FieldEntry* findField(uint8_t id) const;
    void nodeObjectIdFromTopic(char* out, size_t cap, const char* topic) const;

    IMqttClient* m_mqtt;
    IEspNowClient* m_espNow;
    ILogger* m_logger;
    const DeviceInfo& m_info;
};