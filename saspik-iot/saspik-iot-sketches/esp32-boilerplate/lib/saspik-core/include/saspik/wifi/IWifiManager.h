// SPDX-License-Identifier: MIT
/*
 * IWifiManager.h — контракт управления STA-подключением.
 *
 * WifiManager отвечает только за STA-режим (connect/reconnect/авто-ребут);
 * captive portal живёт в отдельном модуле. Интерфейс нужен для моков
 * в host-тестах приложений (пока приложения тестируются только на железе).
 */

#pragma once

#include <stdint.h>

/**
 * @brief Управление WiFi-соединением (STA).
 */
class IWifiManager {
public:
    virtual ~IWifiManager() = default;

    /**
     * @brief Начать подключение к точке доступа (не блокирует).
     * @return true всегда (результат — через isConnected()/update()).
     */
    virtual bool begin(const char* ssid, const char* pass) = 0;

    /** @brief Неблокирующий тик: поддержание связи и авто-ребут. */
    virtual void update() = 0;

    /** @brief true, если подключены к точке доступа. */
    virtual bool isConnected() const = 0;

    /** @brief Уровень сигнала RSSI (dBm). */
    virtual int8_t rssi() const = 0;
};