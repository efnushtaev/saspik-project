// SPDX-License-Identifier: MIT
/*
 * WifiManager.h — STA-менеджер: reconnect и авто-ребут по таймауту.
 *
 * Логика перенесена из esp32-local-mqtt (handleWifiReconnect) без изменений
 * поведения: восстановление не чаще WIFI_RECONNECT_INTERVAL_MS, полный
 * рестарт при непрерывной потере дольше WIFI_REBOOT_TIMEOUT_MS.
 * Заголовок чистый (без Arduino) — тесты могут подменять реализацию.
 */

#pragma once

#include <stdint.h>

#include "log/ILogger.h"
#include "wifi/IWifiManager.h"

/**
 * @brief Поддержание STA-соединения с защитой от зависаний.
 */
class WifiManager : public IWifiManager {
public:
    /**
     * @param logger                опциональный логгер (nullptr разрешён)
     * @param reconnectIntervalMs   интервал попыток восстановления
     * @param rebootTimeoutMs       непрерывная потеря → авто-ребут
     * @param enableRebootWatchdog  false для Sensor (устройство спит, см. TODO)
     */
    WifiManager(ILogger* logger, uint32_t reconnectIntervalMs,
                uint32_t rebootTimeoutMs, bool enableRebootWatchdog);

    bool begin(const char* ssid, const char* pass) override;
    void update() override;
    bool isConnected() const override;
    int8_t rssi() const override;

private:
    void tryRebootIfLost(uint32_t now);

    ILogger* m_logger;
    uint32_t m_reconnectIntervalMs;
    uint32_t m_rebootTimeoutMs;
    bool m_enableRebootWatchdog;
    uint32_t m_lastReconnectMs;
    uint32_t m_lostSinceMs;
};