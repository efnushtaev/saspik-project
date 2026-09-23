#ifndef DHT22_SENSOR_H
#define DHT22_SENSOR_H

#include "ISensor.h"
#include <DHT.h>
#include "config.h"

/**
 * @brief Датчик температуры и влажности DHT-22 (SRP, OCP)
 * 
 * Реализует интерфейс ISensor.
 * Использует фильтрацию методом усечённого среднего.
 */
class Dht22Sensor : public ISensor {
public:
    Dht22Sensor(uint8_t pin);

    // ISensor interface
    void begin() override;
    bool read() override;
    const char* getName() const override;

    /**
     * @brief Получить последнее прочитанное значение температуры
     */
    float getTemperature() const { return _temperature; }

    /**
     * @brief Получить последнее прочитанное значение влажности
     */
    float getHumidity() const { return _humidity; }

private:
    DHT _dht;
    float _temperature = 0.0f;
    float _humidity = 0.0f;
};

#endif // DHT22_SENSOR_H