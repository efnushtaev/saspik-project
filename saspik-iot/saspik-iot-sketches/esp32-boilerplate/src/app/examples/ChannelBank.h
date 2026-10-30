// SPDX-License-Identifier: MIT
/*
 * ChannelBank.h — пользовательские каналы из CHANNELS[] (config.h).
 *
 * Заменяет жёсткий набор USE_EXAMPLE_*: сенсоры и автоматы собираются из
 * одного массива ChannelConfig. Имена каналов произвольные; JSON-ключи
 * совпадают с key/key2, поэтому UI и прошивка не зависят от fieldId.
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include <ArduinoJson.h>
#include <DHT.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

/**
 * @brief Сенсоры и автоматы, собранные из CHANNELS[].
 */
class ChannelBank {
public:
    ChannelBank();
    ~ChannelBank();

    bool begin();

    bool hasSensors() const { return m_sensorCount > 0; }
    bool hasActuators() const { return m_actuatorCount > 0; }

    /** @brief Добавить в doc все валидные измерения: key → value. */
    void readSensors(JsonDocument& doc) const;

    /** @brief Добавить в doc состояние автоматов: key → "ON"|"OFF". */
    void writeState(JsonDocument& doc) const;

    /** @brief Применить команду по имени канала. */
    bool applyCommand(const char* key, bool on);

    /** @brief Применить команду к первому автомату (legacy {"state":...}). */
    bool applyFirst(bool on);

private:
    struct SensorSlot {
        const ChannelConfig* cfg;
        DHT* dht;
    };
    struct ActuatorSlot {
        const ChannelConfig* cfg;
        bool on;
    };

    ActuatorSlot* findActuator(const char* key);

    SensorSlot m_sensors[MAX_CHANNELS];
    size_t m_sensorCount;
    ActuatorSlot m_actuators[MAX_CHANNELS];
    size_t m_actuatorCount;
};

#endif  // ARDUINO
