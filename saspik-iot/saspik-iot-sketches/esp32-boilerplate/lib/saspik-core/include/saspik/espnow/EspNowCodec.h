// SPDX-License-Identifier: MIT
/*
 * EspNowCodec.h — кодирование/декодирование ESP-NOW пакетов.
 *
 * Чистый модуль без Arduino: покрывается host-тестами (env:native, Unity).
 * Формат кадра — см. EspNowTypes.h. Контрольная сумма — CRC-8/MAXIM
 * (отражённый полином 0x31); все шаблоны boilerplate используют один и тот
 * же EspNowCodec, поэтому совместимость обеспечена «из коробки» (при
 * сопряжении со сторонним узлом сверяйте crc8 и раскладку полей).
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "espnow/EspNowTypes.h"

/**
 * @brief Фрейминг пакетов ESP-NOW.
 */
class EspNowCodec {
public:
    /**
     * @brief Сериализация пакета в буфер.
     * @return длина закодированных байт; 0 — ошибка (нет места/нет полей).
     */
    static size_t encode(const EspNowPacket& packet, uint8_t* out, size_t cap);

    /**
     * @brief Разбор пакета из буфера.
     * @return true, если CRC сошёлся и все поля разобраны без выхода за
     *         границы буфера (иначе пакет отбрасывается целиком).
     */
    static bool decode(const uint8_t* data, size_t len, EspNowPacket& out);

    /** @brief CRC-8/MAXIM по всем байтам данных. */
    static uint8_t crc8(const uint8_t* data, size_t len);
};