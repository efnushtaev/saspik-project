// SPDX-License-Identifier: MIT
/*
 * MqttManager.h — реализация IMqttClient поверх WiFiClient + PubSubClient.
 *
 * Транспорт (WiFiClient + PubSubClient) спрятан за PIMPL (struct Impl):
 * заголовок остаётся чистым C++ и не тащит Arduino-заголовки в host-сборку.
 * Сам менеджер — firmware-only: реализация в .cpp обёрнута в #ifdef ARDUINO,
 * на хосте это пустая единица трансляции (тесты используют MockMqttClient).
 *
 * Статический s_instance — единственное исключение из «без глобалов»:
 * PubSubClient вызывает C-колбэк без user-контекста, поэтому адрес активного
 * экземпляра хранится в статике внутри класса. Все вызовы происходят из
 * loop() одного потока — гонок нет (обоснование в плане архитектуры).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "mqtt/IMqttClient.h"

class ILogger;

/**
 * @brief MQTT-клиент на PubSubClient: LWT, диспетчеризация, авто-ребут.
 */
class MqttManager : public IMqttClient {
public:
    /**
     * @param logger опциональный логгер (nullptr разрешён)
     */
    explicit MqttManager(ILogger* logger);
    ~MqttManager() override;

    bool begin(const MqttConfig& cfg) override;
    void update() override;
    bool isConnected() const override;
    bool publish(const char* topic, const char* payload, bool retained) override;
    bool subscribe(const char* topic, IMessageHandler* handler) override;

private:
    /** Одна подписка: копия топика + обработчик (поиск — strcmp). */
    struct Subscription {
        char topic[MQTT_TOPIC_MAX_LEN];
        IMessageHandler* handler;
    };

    /** WiFiClient + PubSubClient — Arduino-типы, определены в .cpp. */
    struct Impl;

    static void onMessageCallback(char* topic, uint8_t* payload, size_t length);
    void dispatchMessage(const char* topic, const uint8_t* payload,
                         size_t length);
    void tryConnect(uint32_t now);
    void tryRebootIfBrokerLost(uint32_t now);
    void resubscribeAll();
    Subscription* findSubscription(const char* topic);
    // Совпадение топика с шаблоном подписки: поддерживается один уровень '+'
    // (device/{unitId}/+); '#' (multi-level) в v1 не используется.
    static bool topicMatches(const char* pattern, const char* topic);

    static MqttManager* s_instance;  // только для C-колбэка PubSubClient

    ILogger* m_logger;
    MqttConfig m_cfg;
    Impl* m_impl;
    bool m_configured;
    uint32_t m_lastConnectAttemptMs;
    uint32_t m_brokerLostSinceMs;
    uint8_t m_subscriptionCount;
    Subscription m_subscriptions[MQTT_SUBSCRIPTIONS_MAX];
};