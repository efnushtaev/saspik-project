// SPDX-License-Identifier: MIT
/*
 * ISensor.h — контракт любого датчика (примеры: DHT22, геркон; шинные —
 * через InterfaceManager).
 *
 * Датчик возвращает SensorSample с fieldId из config.h — благодаря этому
 * мост ESP-NOW↔MQTT и логика приложения не зависят от конкретного железа
 * (OCP: новый датчик = новая реализация ISensor, ноль правок в мосте).
 */

#pragma once

#include "interfaces/sensor/SensorSample.h"

/**
 * @brief Читаемый датчик.
 */
class ISensor {
public:
    virtual ~ISensor() = default;

    /**
     * @brief Инициализация датчика (пины/шины уже настроены).
     * @return false, если датчик не отвечает (пример всё равно продолжит
     *         работу, публикуя valid=false)
     */
    virtual bool begin() = 0;

    /**
     * @brief Прочитать измерение.
     * @return sample; sample.valid=false — чтение не удалось
     */
    virtual SensorSample read() = 0;
};