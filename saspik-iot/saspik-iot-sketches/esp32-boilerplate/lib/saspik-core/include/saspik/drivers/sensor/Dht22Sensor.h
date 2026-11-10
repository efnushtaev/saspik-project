// SPDX-License-Identifier: MIT
/*
 * Dht22Sensor.h — DHT22 как ISensor: температура + влажность.
 *
 * DHT22 даёт два поля, а ISensor::read() возвращает одно измерение.
 * read() чередует поля: один вызов — температура, следующий — влажность;
 * потребитель (например, ChannelBank) вызывает read() дважды, чтобы получить
 * оба поля. Библиотека DHT кэширует физическое чтение на MIN_INTERVAL (~2 с),
 * поэтому датчик читается один раз. m_mock — псевдозначения без железа.
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include <DHT.h>
#include <stdint.h>

#include "config.h"
#include "interfaces/sensor/ISensor.h"

/**
 * @brief DHT22: два поля (температура/влажность) через ISensor.
 */
class Dht22Sensor : public ISensor {
public:
    /**
     * @param pin  пин данных DHT22
     * @param mock true — псевдозначения без железа
     */
    explicit Dht22Sensor(uint8_t pin, bool mock = false);

    bool begin() override;
    SensorSample read() override;

private:
    SensorSample readMock();
    SensorSample readReal();

    DHT m_dht;
    bool m_mock;
    bool m_alternate;  ///< true — вернуть температуру, false — влажность
};

#endif  // ARDUINO
