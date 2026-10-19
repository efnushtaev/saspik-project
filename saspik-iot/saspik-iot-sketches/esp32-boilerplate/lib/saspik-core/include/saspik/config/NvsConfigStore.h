// SPDX-License-Identifier: MIT
/*
 * NvsConfigStore.h — реализация IConfigStore поверх Preferences (NVS).
 *
 * Отдельный namespace "saspik_app" намеренно НЕ совпадает с namespace "saspik",
 * который использует shared/wifi-config: портал и параметры приложения не
 * должны затирать друг друга (разные владельцы, разные форматы).
 */

#pragma once

#ifdef ARDUINO
#include <Preferences.h>
#endif

#include "config/IConfigStore.h"

/**
 * @brief Хранилище runtime-параметров приложения в NVS.
 */
class NvsConfigStore : public IConfigStore {
public:
    static constexpr const char* kNamespace = "saspik_app";

    bool begin() override;
    bool getUInt32(const char* key, uint32_t* out, uint32_t def) override;
    bool getUInt8(const char* key, uint8_t* out, uint8_t def) override;
    bool getBytes(const char* key, uint8_t* out, size_t outSize) override;
    bool putUInt32(const char* key, uint32_t value) override;
    bool putUInt8(const char* key, uint8_t value) override;
    bool putBytes(const char* key, const uint8_t* data, size_t size) override;

private:
#ifdef ARDUINO
    Preferences m_prefs;
#endif
};