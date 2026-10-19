// SPDX-License-Identifier: MIT
/*
 * MockConfigStore.h — мок IConfigStore поверх std::map.
 *
 * Повторяет семантику NVS: отсутствующий ключ → false + значение по
 * умолчанию. Используется тестами ConfigManager (merge defaults ← NVS).
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include <string.h>

#include "config/IConfigStore.h"

class MockConfigStore : public IConfigStore {
public:
    bool begin() override { return true; }

    bool getUInt32(const char* key, uint32_t* out, uint32_t def) override {
        const auto it = m_u32.find(key);
        if (it == m_u32.end()) {
            *out = def;
            return false;
        }
        *out = it->second;
        return true;
    }

    bool getUInt8(const char* key, uint8_t* out, uint8_t def) override {
        const auto it = m_u8.find(key);
        if (it == m_u8.end()) {
            *out = def;
            return false;
        }
        *out = it->second;
        return true;
    }

    bool getBytes(const char* key, uint8_t* out, size_t outSize) override {
        const auto it = m_bytes.find(key);
        if (it == m_bytes.end() || it->second.size() != outSize) {
            return false;
        }
        memcpy(out, it->second.data(), outSize);
        return true;
    }

    bool putUInt32(const char* key, uint32_t value) override {
        m_u32[key] = value;
        return true;
    }

    bool putUInt8(const char* key, uint8_t value) override {
        m_u8[key] = value;
        return true;
    }

    bool putBytes(const char* key, const uint8_t* data, size_t size) override {
        m_bytes[key].assign(data, data + size);
        return true;
    }

    // ---- Хелперы для тестов ----
    void setUInt32(const char* key, uint32_t value) { m_u32[key] = value; }
    void setUInt8(const char* key, uint8_t value) { m_u8[key] = value; }
    void setBytes(const char* key, const uint8_t* data, size_t size) {
        m_bytes[key].assign(data, data + size);
    }
    void clear() {
        m_u32.clear();
        m_u8.clear();
        m_bytes.clear();
    }

private:
    std::map<std::string, uint32_t> m_u32;
    std::map<std::string, uint8_t> m_u8;
    std::map<std::string, std::vector<uint8_t>> m_bytes;
};