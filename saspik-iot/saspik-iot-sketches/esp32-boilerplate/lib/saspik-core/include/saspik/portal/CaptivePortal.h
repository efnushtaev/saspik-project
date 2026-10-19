// SPDX-License-Identifier: MIT
/*
 * CaptivePortal.h — реализация ICaptivePortal поверх WifiConfigManager.
 *
 * Внутри живёт документированный синглтон ::WifiConfig из shared/wifi-config:
 * менять библиотеку нельзя («переиспользуем как есть»), поэтому фасад просто
 * делегирует ей, а весь остальной код про глобальный объект не знает.
 */

#pragma once

#ifdef ARDUINO

#include "portal/ICaptivePortal.h"

/**
 * @brief Тонкий фасад над WifiConfigManager (shared/wifi-config).
 */
class CaptivePortal : public ICaptivePortal {
public:
    bool begin(DeviceConfig& out, int8_t buttonPin,
               const DeviceConfig* defaults) override;
    void handlePortal() override;
    bool isPortalMode() const override;
};

#endif  // ARDUINO