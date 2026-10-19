// SPDX-License-Identifier: MIT
/*
 * test_config_manager.cpp — host-тесты ConfigManager (merge defaults ← NVS).
 *
 * Правило: дефолты из config.h, переопределения из NVS побеждают; отсутствие
 * ключа в NVS молча возвращает поведение к дефолту.
 */

#include <string.h>

#include <unity.h>

#include "MockConfigStore.h"
#include "config/ConfigManager.h"

namespace {
MockConfigStore store;
ConfigManager* manager = nullptr;
}  // namespace

void setUp() {
    store.clear();
    manager = new ConfigManager(store);
}

void tearDown() {
    delete manager;
    manager = nullptr;
}

void test_load_defaults_from_config_h() {
    const AppConfig cfg = manager->loadDefaults();
    TEST_ASSERT_EQUAL_UINT32(SENSOR_INTERVAL_MS, cfg.sensorIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(SLEEP_INTERVAL_MS, cfg.sleepIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(DIAG_PUBLISH_INTERVAL_MS, cfg.diagIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(ESP_NOW_CHANNEL, cfg.espNowChannel);
    // v1: единственный peer по умолчанию — шлюз из config.h.
    TEST_ASSERT_EQUAL_UINT8(1, cfg.peerCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(DEFAULT_GATEWAY_MAC, cfg.peers[0].mac, 6);
}

void test_load_with_empty_store_returns_defaults() {
    const AppConfig cfg = manager->load();
    TEST_ASSERT_EQUAL_UINT32(SENSOR_INTERVAL_MS, cfg.sensorIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(1, cfg.peerCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(DEFAULT_GATEWAY_MAC, cfg.peers[0].mac, 6);
}

void test_load_nvs_overrides_defaults() {
    store.setUInt32("sensor_ms", 5000);
    store.setUInt32("sleep_ms", 120000);
    store.setUInt8("ch", 6);

    const AppConfig cfg = manager->load();
    TEST_ASSERT_EQUAL_UINT32(5000, cfg.sensorIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(120000, cfg.sleepIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(6, cfg.espNowChannel);
    // Незатронутые ключи остаются дефолтными.
    TEST_ASSERT_EQUAL_UINT32(DIAG_PUBLISH_INTERVAL_MS, cfg.diagIntervalMs);
}

void test_save_then_load_roundtrip() {
    AppConfig cfg = manager->loadDefaults();
    cfg.sensorIntervalMs = 7777;
    cfg.espNowChannel = 3;
    TEST_ASSERT_TRUE(manager->save(cfg));

    const AppConfig loaded = manager->load();
    TEST_ASSERT_EQUAL_UINT32(7777, loaded.sensorIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(3, loaded.espNowChannel);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(DEFAULT_GATEWAY_MAC, loaded.peers[0].mac, 6);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_load_defaults_from_config_h);
    RUN_TEST(test_load_with_empty_store_returns_defaults);
    RUN_TEST(test_load_nvs_overrides_defaults);
    RUN_TEST(test_save_then_load_roundtrip);
    return UNITY_END();
}