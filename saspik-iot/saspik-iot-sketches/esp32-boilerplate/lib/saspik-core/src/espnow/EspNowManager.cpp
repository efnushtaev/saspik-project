// SPDX-License-Identifier: MIT
/*
 * EspNowManager.cpp — реализация на esp_now API.
 *
 * begin(): WiFi.mode(WIFI_STA) — ESP-NOW работает и без подключения к точке
 * доступа, но канал должен совпадать с каналом сети шлюза (ESP_NOW_CHANNEL
 * в config.h; автосинхронизация — открытый вопрос архитектуры).
 * Пиры регистрируются через addPeer() из NODE_TABLE / DEFAULT_GATEWAY_MAC
 * (v1 — фиксированный список, runtime-управление — TODO, см. TODO.md).
 */

#include "espnow/EspNowManager.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#include "espnow/EspNowCodec.h"
#include "log/ILogger.h"  // заголовок даёт только forward-declaration

namespace {
// Колбэк статуса отправки: v1 статус не трактует (retry/ACK — TODO).
// Свободная функция, а не метод: её сигнатура должна точно совпадать с
// esp_now_send_cb_t (esp_now_send_status_t), а заголовок класса — чистый.
void onSendCb(const uint8_t* mac, esp_now_send_status_t status) {
    (void)mac;
    (void)status;
}
}  // namespace

EspNowManager* EspNowManager::s_instance = nullptr;

EspNowManager::EspNowManager(ILogger* logger)
    : m_logger(logger),
      m_listener(nullptr),
      m_ready(false),
      m_peerCount(0),
      m_rxLen(0),
      m_rxPending(false) {
    memset(m_rxMac, 0, sizeof(m_rxMac));
    memset(m_rxBuffer, 0, sizeof(m_rxBuffer));
}

bool EspNowManager::begin(uint8_t channel) {
    if (m_ready) {
        return true;
    }
    WiFi.mode(WIFI_STA);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        if (m_logger != nullptr) {
            m_logger->logError("espnow", "init failed");
        }
        return false;
    }
    esp_now_register_recv_cb(&EspNowManager::onRecvCb);
    esp_now_register_send_cb(&onSendCb);
    s_instance = this;
    m_ready = true;
    return true;
}

bool EspNowManager::addPeer(const uint8_t* mac) {
    if (!m_ready || mac == nullptr) {
        return false;
    }
    if (m_peerCount >= ESP_NOW_MAX_PEERS) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "peers full");
        }
        return false;
    }
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;       // 0 = текущий канал (задан в begin)
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;   // v1 без шифрования (общий ключ — TODO)
    if (esp_now_add_peer(&peer) != ESP_OK) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "add peer failed");
        }
        return false;
    }
    ++m_peerCount;
    return true;
}

void EspNowManager::update() {
    if (!m_ready) {
        return;
    }
    dispatchPending();
}

bool EspNowManager::send(const uint8_t* mac, const uint8_t* data, size_t len) {
    if (!m_ready || mac == nullptr || data == nullptr || len == 0 ||
        len > ESP_NOW_PACKET_MAX) {
        return false;
    }
    if (esp_now_send(mac, data, len) != ESP_OK) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "send failed");
        }
        return false;
    }
    return true;
}

void EspNowManager::setListener(IEspNowListener* listener) {
    m_listener = listener;
}

void EspNowManager::dispatchPending() {
    if (!m_rxPending) {
        return;
    }
    m_rxPending = false;
    if (m_listener == nullptr) {
        return;  // приняли, а слушать некому — пакет отбрасывается
    }
    EspNowPacket packet;
    if (!EspNowCodec::decode(m_rxBuffer, m_rxLen, packet)) {
        if (m_logger != nullptr) {
            m_logger->logWarn("espnow", "bad packet dropped");
        }
        return;
    }
    m_listener->onEspNowPacket(m_rxMac, packet);
}

void EspNowManager::onRecvCb(const uint8_t* mac, const uint8_t* data, int len) {
    if (s_instance == nullptr || mac == nullptr || data == nullptr) {
        return;
    }
    if (len < 0 || static_cast<size_t>(len) > ESP_NOW_PACKET_MAX) {
        return;
    }
    // Однослотовый буфер: пачка пакетов перезаписывает предыдущий
    // (drop-oldest). Для v1 достаточно: интервалы датчиков ≫ времени loop.
    s_instance->m_rxLen = static_cast<size_t>(len);
    memcpy(s_instance->m_rxBuffer, data, s_instance->m_rxLen);
    memcpy(s_instance->m_rxMac, mac, 6);
    s_instance->m_rxPending = true;
}

#endif  // ARDUINO