// SPDX-License-Identifier: MIT
/*
 * MockWifiManager.h — мок IWifiManager для host-тестов.
 *
 * Заготовка: приложения (Composition Root) тестируются на железе, но мок
 * нужен, чтобы будущие тесты приложений не зависели от WiFi-стека.
 */

#pragma once

#include "wifi/IWifiManager.h"

class MockWifiManager : public IWifiManager {
public:
    bool begin(const char* ssid, const char* pass) override {
        m_ssid = ssid;
        m_pass = pass;
        return true;
    }

    void update() override {}

    bool isConnected() const override { return m_connected; }

    int8_t rssi() const override { return m_rssi; }

    // ---- Хелперы для тестов ----
    void setConnected(bool connected) { m_connected = connected; }
    const char* ssid() const { return m_ssid; }
    const char* pass() const { return m_pass; }

private:
    const char* m_ssid = nullptr;
    const char* m_pass = nullptr;
    bool m_connected = false;
    int8_t m_rssi = -60;
};