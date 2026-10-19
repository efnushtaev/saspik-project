// SPDX-License-Identifier: MIT
/*
 * EspNowTypes.h — типы ESP-NOW протокола boilerplate.
 *
 * Проводной формат (v2, утверждён в plans/boilerplate-architecture.md):
 *   [type(1)] [timestamp LE(4)] [fieldId(1) + len(1) + data(len)]* [crc8(1)]
 * Отличие от v1 (saspik-iot-node-sensor-1): добавлены байт типа и длина поля —
 * шлюз различает данные/логи/команды/ACK и может передавать произвольные
 * блобы (JSON лога/команды в служебных полях 0xFF/0xFE). Лимит пакета —
 * стандартные для ESP-NOW 250 байт (ESP_NOW_PACKET_MAX).
 */

#pragma once

#include <stdint.h>

#include "config.h"

/** @brief Поле данных внутри ESP-NOW пакета. */
struct EspNowField {
    uint8_t id;           ///< fieldId (FIELD_DICTIONARY или 0xFF/0xFE)
    const uint8_t* data;  ///< указатель на данные (НЕ владеет памятью)
    uint8_t len;          ///< длина данных (пакет ≤ 250 байт)
};

/** @brief Декодированный ESP-NOW пакет. */
struct EspNowPacket {
    uint8_t type;                  ///< ESP_NOW_TYPE_* (config.h)
    uint32_t timestampMs;          ///< uptime источника (диагностика)
    EspNowField fields[ESP_NOW_MAX_FIELDS];
    uint8_t fieldCount;
};