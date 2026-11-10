// SPDX-License-Identifier: MIT
/*
 * Dht22Sensor.cpp — реализация DHT22 через ISensor.
 */

#include "drivers/sensor/Dht22Sensor.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <math.h>

Dht22Sensor::Dht22Sensor(uint8_t pin, bool mock)
    : m_dht(pin, DHT_TYPE), m_mock(mock), m_alternate(true) {}

bool Dht22Sensor::begin() {
    m_dht.begin();
    return true;
}

SensorSample Dht22Sensor::read() {
    if (m_mock) {
        return readMock();
    }
    return readReal();
}

SensorSample Dht22Sensor::readMock() {
    // Псевдозначения для отладки конвейера без датчика.
    const float now = static_cast<float>(millis()) / 1000.0f;
    SensorSample sample = {};
    sample.valid = true;
    if (m_alternate) {
        sample.fieldId = FIELD_ID_TEMPERATURE;
        sample.value = 23.0f + 3.0f * sinf(now * 0.1f);
    } else {
        sample.fieldId = FIELD_ID_HUMIDITY;
        sample.value = 45.0f + 8.0f * sinf(now * 0.07f + 2.0f);
    }
    m_alternate = !m_alternate;
    return sample;
}

SensorSample Dht22Sensor::readReal() {
    SensorSample sample = {};
    const float temperature = m_dht.readTemperature();
    const float humidity = m_dht.readHumidity();
    if (isnan(temperature) || isnan(humidity)) {
        // Датчик не готов (первые секунды после begin или обрыв линии):
        // valid=false — приложение пропустит измерение, а не опубликует nan.
        return sample;
    }
    sample.valid = true;
    if (m_alternate) {
        sample.fieldId = FIELD_ID_TEMPERATURE;
        sample.value = temperature;
    } else {
        sample.fieldId = FIELD_ID_HUMIDITY;
        sample.value = humidity;
    }
    m_alternate = !m_alternate;
    return sample;
}

#endif  // ARDUINO
