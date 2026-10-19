// SPDX-License-Identifier: MIT
/*
 * SensorSample.h — результат одного измерения.
 *
 * Value type без логики: одинаковый для всех датчиков, поэтому мост и
 * приложения работают с любым ISensor одинаково. v1 — только float
 * (совместимо с FIELD_DICTIONARY в config.h); int/bool/string — TODO.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Одно измерение датчика.
 */
struct SensorSample {
    uint8_t fieldId;  ///< FIELD_ID_* из config.h (ключ в FIELD_DICTIONARY)
    float value;      ///< измеренное значение
    bool valid;       ///< false — чтение не удалось (датчик не готов)
};