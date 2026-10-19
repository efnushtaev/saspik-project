// SPDX-License-Identifier: MIT
/*
 * ConfigManager.h — единственный владелец runtime-параметров приложения.
 *
 * Merge-правило: дефолты из config.h ← переопределения из NVS (NVS побеждает).
 * Это даёт два эффекта: config.h остаётся единственным местом «заводских»
 * значений, а NVS — единственным местом runtime-изменений, причём удаление
 * ключа из NVS молча возвращает поведение к дефолту (никаких «застрявших» значений).
 */

#pragma once

#include "config/AppConfig.h"
#include "config/IConfigStore.h"

/**
 * @brief Загрузка/сохранение AppConfig: defaults(config.h) + NVS overrides.
 */
class ConfigManager {
public:
    /**
     * @param store хранилище NVS (владеется App, передаётся через DI)
     */
    explicit ConfigManager(IConfigStore& store);

    /**
     * @brief Конфиг только из compile-time констант config.h.
     */
    AppConfig loadDefaults() const;

    /**
     * @brief Конфиг по правилу defaults ← NVS (NVS побеждает).
     */
    AppConfig load() const;

    /**
     * @brief Сохраняет текущий конфиг в NVS (для runtime-переопределений, v2).
     */
    bool save(const AppConfig& cfg);

private:
    IConfigStore& m_store;

    // Ключи NVS — часть контракта хранилища, поэтому здесь, а не в вызывающем коде.
    static constexpr const char* kKeySensorMs = "sensor_ms";
    static constexpr const char* kKeySleepMs = "sleep_ms";
    static constexpr const char* kKeyDiagMs = "diag_ms";
    static constexpr const char* kKeyChannel = "ch";
    static constexpr const char* kKeyPeers = "peers";
    static constexpr const char* kKeyPeerCount = "peer_cnt";
};