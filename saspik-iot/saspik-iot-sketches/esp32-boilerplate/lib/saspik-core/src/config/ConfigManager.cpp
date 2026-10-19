// SPDX-License-Identifier: MIT
/*
 * ConfigManager.cpp — реализация merge-логики конфигурации.
 *
 * Чистый модуль (без Arduino): зависит только от IConfigStore и config.h,
 * поэтому покрывается host-тестами (test_config_manager.cpp).
 */

#include "config/ConfigManager.h"

#include <string.h>

ConfigManager::ConfigManager(IConfigStore& store) : m_store(store) {}

AppConfig ConfigManager::loadDefaults() const {
    AppConfig cfg;
    // v1: единственный peer по умолчанию — шлюз (MAC из config.h).
    // Расширение списка пиров в рантайме — TODO (см. TODO.md).
    memcpy(cfg.peers[0].mac, DEFAULT_GATEWAY_MAC, sizeof(cfg.peers[0].mac));
    cfg.peerCount = 1;
    return cfg;
}

AppConfig ConfigManager::load() const {
    AppConfig cfg = loadDefaults();
    m_store.getUInt32(kKeySensorMs, &cfg.sensorIntervalMs, cfg.sensorIntervalMs);
    m_store.getUInt32(kKeySleepMs, &cfg.sleepIntervalMs, cfg.sleepIntervalMs);
    m_store.getUInt32(kKeyDiagMs, &cfg.diagIntervalMs, cfg.diagIntervalMs);
    m_store.getUInt8(kKeyChannel, &cfg.espNowChannel, cfg.espNowChannel);
    m_store.getUInt8(kKeyPeerCount, &cfg.peerCount, cfg.peerCount);
    // Пиры хранятся одним блоком; если блока нет — остаются дефолты.
    m_store.getBytes(kKeyPeers, reinterpret_cast<uint8_t*>(cfg.peers),
                     sizeof(cfg.peers));
    return cfg;
}

bool ConfigManager::save(const AppConfig& cfg) {
    bool ok = true;
    ok &= m_store.putUInt32(kKeySensorMs, cfg.sensorIntervalMs);
    ok &= m_store.putUInt32(kKeySleepMs, cfg.sleepIntervalMs);
    ok &= m_store.putUInt32(kKeyDiagMs, cfg.diagIntervalMs);
    ok &= m_store.putUInt8(kKeyChannel, cfg.espNowChannel);
    ok &= m_store.putUInt8(kKeyPeerCount, cfg.peerCount);
    ok &= m_store.putBytes(kKeyPeers, reinterpret_cast<const uint8_t*>(cfg.peers),
                           sizeof(cfg.peers));
    return ok;
}