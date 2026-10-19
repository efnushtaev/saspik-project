// SPDX-License-Identifier: MIT
/*
 * NvsConfigStore.cpp — Preferences-реализация хранилища.
 *
 * Тело обёрнуто в #ifdef ARDUINO: в env:native этот файл компилируется как
 * пустая единица трансляции (Preferences на хосте не существует), а тесты
 * используют MockConfigStore. Прошивка получает полноценную реализацию.
 */

#include "config/NvsConfigStore.h"

#ifdef ARDUINO

bool NvsConfigStore::begin() { return m_prefs.begin(kNamespace, false); }

bool NvsConfigStore::getUInt32(const char* key, uint32_t* out, uint32_t def) {
    *out = m_prefs.getUInt(key, def);
    return m_prefs.isKey(key);
}

bool NvsConfigStore::getUInt8(const char* key, uint8_t* out, uint8_t def) {
    *out = static_cast<uint8_t>(m_prefs.getUChar(key, def));
    return m_prefs.isKey(key);
}

bool NvsConfigStore::getBytes(const char* key, uint8_t* out, size_t outSize) {
    if (m_prefs.getBytesLength(key) != static_cast<size_t>(outSize)) {
        return false;
    }
    return m_prefs.getBytes(key, out, outSize) == static_cast<size_t>(outSize);
}

bool NvsConfigStore::putUInt32(const char* key, uint32_t value) {
    return m_prefs.putUInt(key, value) > 0;
}

bool NvsConfigStore::putUInt8(const char* key, uint8_t value) {
    return m_prefs.putUChar(key, value) > 0;
}

bool NvsConfigStore::putBytes(const char* key, const uint8_t* data, size_t size) {
    return m_prefs.putBytes(key, data, size) == static_cast<size_t>(size);
}

#endif  // ARDUINO