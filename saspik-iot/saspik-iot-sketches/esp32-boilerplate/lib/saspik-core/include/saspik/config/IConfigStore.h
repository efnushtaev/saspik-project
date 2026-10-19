// SPDX-License-Identifier: MIT
/*
 * IConfigStore.h — абстракция энергонезависимого хранилища «ключ-значение».
 *
 * На устройстве реализуется поверх Preferences (NVS), в host-тестах — поверх
 * обычного map. Выделена в интерфейс, чтобы merge-логика ConfigManager
 * тестировалась на хосте без железа и без Arduino-зависимостей.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Доступ к NVS-подобному хранилищу по ключам (SRP: только хранение).
 */
class IConfigStore {
public:
    virtual ~IConfigStore() = default;

    /** @brief Открывает хранилище (namespace). true при успехе. */
    virtual bool begin() = 0;

    /**
     * @brief Читает uint32 по ключу.
     * @param out значение (если ключ найден)
     * @param def значение по умолчанию (если ключа нет)
     * @return true, если ключ найден.
     */
    virtual bool getUInt32(const char* key, uint32_t* out, uint32_t def) = 0;

    /** @brief Аналог getUInt32 для uint8_t. */
    virtual bool getUInt8(const char* key, uint8_t* out, uint8_t def) = 0;

    /**
     * @brief Читает блок байт по ключу.
     * @return true, если ключ найден и размер совпал с outSize.
     */
    virtual bool getBytes(const char* key, uint8_t* out, size_t outSize) = 0;

    /** @brief Пишет uint32 по ключу. */
    virtual bool putUInt32(const char* key, uint32_t value) = 0;

    /** @brief Аналог putUInt32 для uint8_t. */
    virtual bool putUInt8(const char* key, uint8_t value) = 0;

    /** @brief Пишет блок байт по ключу. */
    virtual bool putBytes(const char* key, const uint8_t* data, size_t size) = 0;
};