// SPDX-License-Identifier: MIT
/*
 * MockEspNowClient.h — мок IEspNowClient для host-тестов.
 *
 * Записывает последнюю отправку (MAC + сырые байты) и умеет «впрыснуть»
 * входящий пакет слушателю — так тесты проверяют мост и приём команд
 * без радио.
 */

#pragma once

#include <string.h>

#include "espnow/IEspNowClient.h"

class MockEspNowClient : public IEspNowClient {
public:
    bool begin(uint8_t channel) override {
        m_channel = channel;
        m_ready = true;
        return true;
    }

    bool addPeer(const uint8_t* mac) override {
        if (m_peerCount >= 4) {
            return false;
        }
        memcpy(m_peers[m_peerCount], mac, 6);
        ++m_peerCount;
        return true;
    }

    void update() override {}

    bool send(const uint8_t* mac, const uint8_t* data, size_t len) override {
        if (!m_ready || mac == nullptr || data == nullptr || len == 0) {
            return false;
        }
        memcpy(m_lastMac, mac, 6);
        m_lastLen = len;
        memcpy(m_lastData, data, len);
        ++m_sendCount;
        return true;
    }

    void setListener(IEspNowListener* listener) override { m_listener = listener; }

    // ---- Хелперы для тестов ----
    /** @brief Сброс состояния между тестами (мок — глобальный объект). */
    void reset() {
        m_ready = false;
        m_channel = 0;
        m_listener = nullptr;
        m_peerCount = 0;
        m_lastLen = 0;
        m_sendCount = 0;
    }

    void inject(const uint8_t* mac, const EspNowPacket& packet) {
        if (m_listener != nullptr) {
            m_listener->onEspNowPacket(mac, packet);
        }
    }

    uint8_t channel() const { return m_channel; }
    const uint8_t* lastMac() const { return m_lastMac; }
    const uint8_t* lastData() const { return m_lastData; }
    size_t lastLen() const { return m_lastLen; }
    uint32_t sendCount() const { return m_sendCount; }
    uint8_t peerCount() const { return m_peerCount; }

private:
    bool m_ready = false;
    uint8_t m_channel = 0;
    IEspNowListener* m_listener = nullptr;
    uint8_t m_peers[4][6] = {};
    uint8_t m_peerCount = 0;
    uint8_t m_lastMac[6] = {};
    uint8_t m_lastData[250] = {};
    size_t m_lastLen = 0;
    uint32_t m_sendCount = 0;
};