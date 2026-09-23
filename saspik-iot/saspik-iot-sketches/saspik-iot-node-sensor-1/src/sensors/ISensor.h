#ifndef I_SENSOR_H
#define I_SENSOR_H

#include <Arduino.h>

/**
 * @brief Абстрактный интерфейс для всех датчиков (ISP, DIP)
 * 
 * Каждый датчик реализует два метода:
 * - begin()  — инициализация (пины, библиотеки)
 * - read()   — однократное чтение, возвращает true при успехе
 * - getName() — имя датчика для логирования
 */
class ISensor {
public:
    virtual ~ISensor() = default;

    /**
     * @brief Инициализация датчика
     */
    virtual void begin() = 0;

    /**
     * @brief Однократное чтение данных с датчика
     * @return true если чтение успешно, false в противном случае
     */
    virtual bool read() = 0;

    /**
     * @brief Имя датчика (для логирования)
     */
    virtual const char* getName() const = 0;
};

#endif // I_SENSOR_H