// SPDX-License-Identifier: MIT
/*
 * DeviceInfo.h — идентичность объекта (unitId + objectId).
 *
 * Value type без логики: хранит подпись устройства для топиков и лог-конвертов.
 * Выделен в отдельный тип, чтобы Logger/Bridge/TopicBuilder принимали один
 * аргумент вместо двух строк и не зависели от того, где эти строки живут.
 */

#pragma once

#include <cstddef>
#include <cstring>

#include "config.h"

/**
 * @brief Идентичность устройства: unitId и objectId (шифр по classification.md).
 */
class DeviceInfo {
public:
    /**
     * @param unitId   идентификатор юнита (копируется, обрезается по лимиту)
     * @param objectId идентификатор объекта (шифр saspik-...)
     */
    DeviceInfo(const char* unitId, const char* objectId) {
        copyTruncated(m_unitId, unitId, sizeof(m_unitId));
        copyTruncated(m_objectId, objectId, sizeof(m_objectId));
    }

    /** @brief Идентификатор юнита (C-строка). */
    const char* unitId() const { return m_unitId; }

    /** @brief Идентификатор объекта (C-строка). */
    const char* objectId() const { return m_objectId; }

private:
    char m_unitId[DEVICE_INFO_UNIT_LEN];
    char m_objectId[DEVICE_INFO_OBJECT_LEN];

    // Безопасное копирование строки в фиксированный буфер: строки из config.h
    // известны до компиляции, но защита от переполнения дешевле, чем её отсутствие.
    static void copyTruncated(char* dst, const char* src, size_t cap) {
        if (src == nullptr) {
            dst[0] = '\0';
            return;
        }
        strncpy(dst, src, cap - 1);
        dst[cap - 1] = '\0';
    }
};