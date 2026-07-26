#include "Dht22Sensor.h"
#include <algorithm>

Dht22Sensor::Dht22Sensor(uint8_t pin)
    : _dht(pin, DHT22)
{
}

void Dht22Sensor::begin()
{
    _dht.begin();
    delay(STAB_DELAY);
}

bool Dht22Sensor::read()
{
    float tempReadings[SAMPLES];
    float humReadings[SAMPLES];

    for (int i = 0; i < SAMPLES; i++)
    {
        float t = _dht.readTemperature();
        float h = _dht.readHumidity();

        if (isnan(t) || isnan(h))
        {
            Serial.print(getName());
            Serial.println(": ошибка чтения!");
            _temperature = 0;
            _humidity = 0;
            return false;
        }

        tempReadings[i] = t;
        humReadings[i] = h;
        delay(SAMPLE_DELAY);
    }

    // Усечённое среднее: сортируем и отбрасываем крайние 2+2 значения
    std::sort(tempReadings, tempReadings + SAMPLES);
    std::sort(humReadings, humReadings + SAMPLES);

    float tempSum = 0, humSum = 0;
    for (int i = 2; i < SAMPLES - 2; i++)
    {
        tempSum += tempReadings[i];
        humSum += humReadings[i];
    }

    _temperature = tempSum / (SAMPLES - 4);
    _humidity = humSum / (SAMPLES - 4);

    return true;
}

const char* Dht22Sensor::getName() const
{
    return "DHT-22";
}