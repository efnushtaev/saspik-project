// SPDX-License-Identifier: MIT
/*
 * AppConfig.h — runtime-параметры приложения.
 *
 * Чистый value type без логики и без Arduino-зависимостей: хранит то, что
 * может быть переопределено в рантайме (NVS), с дефолтами из config.h.
 * WiFi/MQTT-часть (DeviceConfig) здесь намеренно отсутствует — она живёт
 * в shared/wifi-config и управляется captive portal'ом.
 */

#pragma once

#include <stdint.h>

#include "config.h"

/** @brief Один пир ESP-NOW (v1 — шлюз; массив для будущего расширения). */
struct EspNowPeerCfg {
    uint8_t mac[6];
};

/**
 * @brief Runtime-параметры приложения. Дефолты — константы config.h.
 */
struct AppConfig {
    uint32_t sensorIntervalMs = SENSOR_INTERVAL_MS;   ///< Чтение/отправка данных
    uint32_t sleepIntervalMs = SLEEP_INTERVAL_MS;     ///< Light sleep сенсора
    uint32_t diagIntervalMs = DIAG_PUBLISH_INTERVAL_MS;  ///< Период диагностики
    uint8_t espNowChannel = ESP_NOW_CHANNEL;          ///< Канал ESP-NOW
    EspNowPeerCfg peers[ESP_NOW_MAX_PEERS];           ///< Список пиров
    uint8_t peerCount = 0;                            ///< Количество заполненных пиров
};