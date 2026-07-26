#include "FloatSensor.h"
#include "utils/Filter.h"

FloatSensor::FloatSensor(uint8_t pin)
    : _pin(pin)
{
}

void FloatSensor::begin()
{
    pinMode(_pin, INPUT_PULLUP);
}

bool FloatSensor::read()
{
    float value;
    if (!Filter::readDigital(_pin, value))
    {
        Serial.print(getName());
        Serial.println(": ошибка чтения!");
        return false;
    }

    _state = value;

    Serial.print(getName());
    Serial.print(": ");
    if (value > 0.5f)
    {
        Serial.println("ВОДА ЕСТЬ (контакт замкнут)");
    }
    else
    {
        Serial.println("ВОДЫ НЕТ (контакт разомкнут)");
    }

    return true;
}

const char* FloatSensor::getName() const
{
    return "Float sensor";
}