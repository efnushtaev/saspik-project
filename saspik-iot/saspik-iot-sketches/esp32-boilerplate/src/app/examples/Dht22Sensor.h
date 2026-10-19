// SPDX-License-Identifier: MIT
/*
 * Dht22Sensor.h — пример ISensor: температура/влажность DHT22.
 *
 * Показывает, как пользоваться интерфейсом ISensor: датчик не знает ни пинов
 * (приходят в конструкторе), ни того, кто потребляет его измерения. DHT22
 * даёт ДВА поля (температура + влажность), а ISensor::read() возвращает одно
 * измерение — пример чередует поля по чётности вызова (m_alternate). Это
 * осознанное упрощение примера; полноценный multi-field контракт — TODO
 * (см. TODO.md).
 *
 * USE_MOCK_SENSOR=true (config.h) — плавные псевдо-данные на основе millis()
 * (поведение esp32-local-mqtt): конвейер отлаживается без физического датчика.
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include <DHT.h>

#include "config.h"
#include "interfaces/sensor/ISensor.h"

/**
 * @brief Пример датчика DHT22 (температура/влажность).
 */
class Dht22Sensor : public ISensor {
public:
    /**
     * @param pin пин данных (PIN_GPIO_3, см. config.h)
     */
    explicit Dht22Sensor(uint8_t pin);

    bool begin() override;
    SensorSample read() override;

private:
    SensorSample readMock();
    SensorSample readReal();

    DHT m_dht;
    uint8_t m_pin;
    bool m_alternate;  ///< true — вернуть температуру, false — влажность
};

#endif  // ARDUINO