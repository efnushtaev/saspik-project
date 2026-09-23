#ifndef FLOAT_SENSOR_H
#define FLOAT_SENSOR_H

#include "ISensor.h"
#include "config.h"

/**
 * @brief Поплавковый датчик (геркон) (SRP, OCP)
 * 
 * Реализует интерфейс ISensor.
 * Определяет наличие воды по замкнутому/разомкнутому контакту.
 */
class FloatSensor : public ISensor {
public:
    explicit FloatSensor(uint8_t pin);

    // ISensor interface
    void begin() override;
    bool read() override;
    const char* getName() const override;

    /**
     * @brief Получить последнее прочитанное состояние
     * @return 1.0 если вода есть, 0.0 если воды нет
     */
    float getState() const { return _state; }

private:
    uint8_t _pin;
    float _state = 0.0f;
};

#endif // FLOAT_SENSOR_H