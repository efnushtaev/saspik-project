// SPDX-License-Identifier: MIT
/*
 * MockMqttClient.h — мок IMqttClient для host-тестов.
 *
 * Записывает последнюю публикацию/подписку и умеет «доставить» входящее
 * сообщение зарегистрированному обработчику — так тесты проверяют мост
 * и диспетчеризацию без брокера.
 */

#pragma once

#include <string.h>

#include "mqtt/IMqttClient.h"

class MockMqttClient : public IMqttClient {
public:
    bool begin(const MqttConfig& cfg) override {
        m_cfg = cfg;
        m_connected = true;
        return true;
    }

    void update() override {}

    bool isConnected() const override { return m_connected; }

    bool publish(const char* topic, const char* payload,
                 bool retained) override {
        if (!m_connected) {
            return false;
        }
        strncpy(m_lastTopic, topic, sizeof(m_lastTopic) - 1);
        m_lastTopic[sizeof(m_lastTopic) - 1] = '\0';
        strncpy(m_lastPayload, payload, sizeof(m_lastPayload) - 1);
        m_lastPayload[sizeof(m_lastPayload) - 1] = '\0';
        m_lastRetained = retained;
        ++m_publishCount;
        return true;
    }

    bool subscribe(const char* topic, IMessageHandler* handler) override {
        strncpy(m_subTopic, topic, sizeof(m_subTopic) - 1);
        m_subTopic[sizeof(m_subTopic) - 1] = '\0';
        m_subHandler = handler;
        return true;
    }

    // ---- Хелперы для тестов ----
    /** @brief Сброс состояния между тестами (мок — глобальный объект). */
    void reset() {
        m_connected = false;
        m_lastTopic[0] = '\0';
        m_lastPayload[0] = '\0';
        m_lastRetained = false;
        m_publishCount = 0;
        m_subTopic[0] = '\0';
        m_subHandler = nullptr;
    }

    void setConnected(bool connected) { m_connected = connected; }
    const char* lastTopic() const { return m_lastTopic; }
    const char* lastPayload() const { return m_lastPayload; }
    bool lastRetained() const { return m_lastRetained; }
    uint32_t publishCount() const { return m_publishCount; }
    const char* subscribedTopic() const { return m_subTopic; }
    IMessageHandler* subscribedHandler() const { return m_subHandler; }

    /** @brief Симуляция входящего MQTT-сообщения. */
    void deliver(const char* topic, const char* payload) {
        if (m_subHandler != nullptr) {
            m_subHandler->onMessage(topic, payload);
        }
    }

private:
    MqttConfig m_cfg = {};
    bool m_connected = false;
    char m_lastTopic[64] = {};
    char m_lastPayload[256] = {};
    bool m_lastRetained = false;
    uint32_t m_publishCount = 0;
    char m_subTopic[64] = {};
    IMessageHandler* m_subHandler = nullptr;
};