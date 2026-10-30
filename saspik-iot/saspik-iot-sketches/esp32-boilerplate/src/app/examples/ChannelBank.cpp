// SPDX-License-Identifier: MIT
/*
 * ChannelBank.cpp — реализация пользовательских каналов.
 */

#include "app/examples/ChannelBank.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <math.h>
#include <string.h>

ChannelBank::ChannelBank() : m_sensorCount(0), m_actuatorCount(0) {
    for (size_t i = 0; i < MAX_CHANNELS; ++i) {
        m_sensors[i].cfg = nullptr;
        m_sensors[i].dht = nullptr;
        m_actuators[i].cfg = nullptr;
        m_actuators[i].on = false;
    }
}

ChannelBank::~ChannelBank() {
    for (size_t i = 0; i < m_sensorCount; ++i) {
        delete m_sensors[i].dht;
        m_sensors[i].dht = nullptr;
    }
}

bool ChannelBank::begin() {
    for (size_t i = 0; i < CHANNEL_COUNT && i < MAX_CHANNELS; ++i) {
        const ChannelConfig& cfg = CHANNELS[i];
        if (cfg.role == ChannelRole::Sensor) {
            SensorSlot& slot = m_sensors[m_sensorCount++];
            slot.cfg = &cfg;
            slot.dht = nullptr;
            if (cfg.model == ChannelModel::Dht22) {
                slot.dht = new DHT(cfg.pin, DHT_TYPE);
                if (slot.dht != nullptr) {
                    slot.dht->begin();
                }
            } else if (cfg.model == ChannelModel::Float) {
                pinMode(cfg.pin, INPUT_PULLUP);
            }
        } else {
            ActuatorSlot& slot = m_actuators[m_actuatorCount++];
            slot.cfg = &cfg;
            slot.on = false;
            pinMode(cfg.pin, OUTPUT);
            digitalWrite(cfg.pin, cfg.activeLow ? HIGH : LOW);
        }
    }
    return true;
}

void ChannelBank::readSensors(JsonDocument& doc) const {
    const float t = static_cast<float>(millis()) / 1000.0f;
    for (size_t i = 0; i < m_sensorCount; ++i) {
        const ChannelConfig& cfg = *m_sensors[i].cfg;
        if (cfg.model == ChannelModel::Dht22) {
            float temperature;
            float humidity;
            if (cfg.mock) {
                temperature = 23.0f + 3.0f * sinf(t * 0.1f);
                humidity = 45.0f + 8.0f * sinf(t * 0.07f + 2.0f);
            } else {
                if (m_sensors[i].dht == nullptr) {
                    continue;
                }
                temperature = m_sensors[i].dht->readTemperature();
                humidity = m_sensors[i].dht->readHumidity();
                if (isnan(temperature) || isnan(humidity)) {
                    continue;
                }
            }
            doc[cfg.key] = temperature;
            if (cfg.key2 != nullptr) {
                doc[cfg.key2] = humidity;
            }
        } else if (cfg.model == ChannelModel::Float) {
            const bool on = cfg.mock
                                ? (static_cast<uint32_t>(millis() / 5000) % 2 == 0)
                                : (digitalRead(cfg.pin) == LOW);
            doc[cfg.key] = on ? 1.0f : 0.0f;
        }
    }
}

void ChannelBank::writeState(JsonDocument& doc) const {
    for (size_t i = 0; i < m_actuatorCount; ++i) {
        doc[m_actuators[i].cfg->key] = m_actuators[i].on ? "ON" : "OFF";
    }
}

bool ChannelBank::applyCommand(const char* key, bool on) {
    ActuatorSlot* slot = findActuator(key);
    if (slot == nullptr) {
        return false;
    }
    slot->on = on;
    const ChannelConfig& cfg = *slot->cfg;
    if (cfg.activeLow) {
        digitalWrite(cfg.pin, on ? LOW : HIGH);
    } else {
        digitalWrite(cfg.pin, on ? HIGH : LOW);
    }
    return true;
}

bool ChannelBank::applyFirst(bool on) {
    if (m_actuatorCount == 0) {
        return false;
    }
    return applyCommand(m_actuators[0].cfg->key, on);
}

ChannelBank::ActuatorSlot* ChannelBank::findActuator(const char* key) {
    if (key == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < m_actuatorCount; ++i) {
        if (strcmp(m_actuators[i].cfg->key, key) == 0) {
            return &m_actuators[i];
        }
    }
    return nullptr;
}

#endif  // ARDUINO
