// SPDX-License-Identifier: MIT
/*
 * RuntimeConfig.h — полный рантайм-конфиг устройства.
 *
 * Объединяет две независимые части:
 *  - DeviceConfig (shared/wifi-config) — WiFi/MQTT, заполняется CaptivePortal
 *    из NVS через форму портала (библиотека переиспользуется как есть);
 *  - AppConfig — параметры приложения (интервалы, канал, пиры), дефолты
 *    из config.h, переопределения из NVS через ConfigManager.
 *
 * Тип используется только в App (Composition Root); чистые модули библиотеки
 * его не знают, поэтому host-тесты обходятся без WifiConfig.h.
 */

#pragma once

#ifdef ARDUINO

#include "WifiConfig.h"  // DeviceConfig (shared/wifi-config, переиспользуем как есть)
#include "config/AppConfig.h"

/**
 * @brief Рантайм-конфиг устройства: WiFi/MQTT + параметры приложения.
 */
struct RuntimeConfig {
    DeviceConfig wifi = {};  ///< Заполняется CaptivePortal::begin()
    AppConfig app;           ///< Заполняется ConfigManager::load()
};

#endif  // ARDUINO