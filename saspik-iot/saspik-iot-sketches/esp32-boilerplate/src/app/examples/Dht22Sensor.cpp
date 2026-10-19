// SPDX-License-Identifier: MIT
/*
 * Dht22Sensor.cpp — реализация примера DHT22.
 */

#include "app/examples/Dht22Sensor.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <math.h>

Dht22Sensor::Dht22Sensor(uint8_t pin)
    : m_dht(pin, DHT_TYPE), m_pin(pin), m_alternate(true) {}

bool Dht22Sensor::begin() {
    m_dht.begin();
    return true;
}

SensorSample Dht22Sensor::read() {
    if (USE_MOCK_SENSOR) {
        return readMock();
    }
    return readReal();
}

SensorSample Dht22Sensor::readMock() {
    // Мок повторяет esp32-local-mqtt: синусоиды от millis() — значения
    // «живые», но предсказуемые, удобно проверять конвейер глазами.
    const float t = static_cast<float>(millis()) / 1000.0f;
    SensorSample sample = {};
    sample.valid = true;
    if (m_alternate) {
        sample.fieldId = FIELD_ID_TEMPERATURE;
        sample.value = 23.0f + 3.0f * sinf(t * 0.1f);
    } else {
        sample.fieldId = FIELD_ID_HUMIDITY;
        sample.value = 45.0f + 8.0f * sinf(t * 0.07f + 2.0f);
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
        // возвращаем valid=false — приложение пропустит измерение, а не
        // опубликует мусор (nan в JSON не сериализуется корректно).
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