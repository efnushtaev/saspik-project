// SPDX-License-Identifier: MIT
/*
 * WifiManager.cpp — реализация STA-менеджера.
 *
 * #ifdef ARDUINO: использует WiFi.h и ESP.restart(), на хосте — пустая TU.
 */

#include "wifi/WifiManager.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <WiFi.h>

WifiManager::WifiManager(ILogger* logger, uint32_t reconnectIntervalMs,
                         uint32_t rebootTimeoutMs, bool enableRebootWatchdog)
    : m_logger(logger),
      m_reconnectIntervalMs(reconnectIntervalMs),
      m_rebootTimeoutMs(rebootTimeoutMs),
      m_enableRebootWatchdog(enableRebootWatchdog),
      m_lastReconnectMs(0),
      m_lostSinceMs(0) {}

bool WifiManager::begin(const char* ssid, const char* pass) {
    // Если WiFi уже поднят (например, WifiConfig.begin подключил) — не трогаем.
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, pass);
    }
    return true;
}

void WifiManager::update() {
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED) {
        m_lostSinceMs = 0;
        return;
    }
    // Не чаще интервала: частые disconnect/reconnect раскачивают стек WiFi.
    if (now - m_lastReconnectMs < m_reconnectIntervalMs) {
        return;
    }
    m_lastReconnectMs = now;
    if (m_lostSinceMs == 0) {
        m_lostSinceMs = now;
    }
    if (m_logger != nullptr) {
        m_logger->logWarn("wifi", "wifi lost, reconnecting");
    }
    WiFi.disconnect();
    WiFi.reconnect();
    tryRebootIfLost(now);
}

void WifiManager::tryRebootIfLost(uint32_t now) {
    if (!m_enableRebootWatchdog || m_lostSinceMs == 0) {
        return;
    }
    if (now - m_lostSinceMs < m_rebootTimeoutMs) {
        return;
    }
    // Резерв: если WiFi не поднялся за отведённое время — полный рестарт,
    // повторного connect недостаточно (поведение esp32-local-mqtt).
    if (m_logger != nullptr) {
        m_logger->logError("reboot", "wifi-lost");
    }
    delay(100);  // дать уйти конверту в UART/очередь перед рестартом
    ESP.restart();
}

bool WifiManager::isConnected() const { return WiFi.status() == WL_CONNECTED; }

int8_t WifiManager::rssi() const { return WiFi.RSSI(); }

#endif  // ARDUINO