// SPDX-License-Identifier: MIT
/*
 * IActuator.h — контракт исполнителя (пример: реле на PIN_GPIO_5).
 *
 * Уровень задан float 0..1: реле трактует как on/off (>= 0.5), диммер —
 * как скважность ШИМ. Единая семантика позволяет обрабатывать команды
 * из MQTT одинаково для любых исполнителей (OCP).
 */

#pragma once

/**
 * @brief Управляемый исполнитель.
 */
class IActuator {
public:
    virtual ~IActuator() = default;

    /**
     * @brief Инициализация исполнителя (пин уже известен из config.h).
     */
    virtual bool begin() = 0;

    /**
     * @brief Применить состояние.
     * @param level уровень 0..1 (за пределами — обрезается)
     * @return false, если применить не удалось
     */
    virtual bool set(float level) = 0;

    /** @brief Текущий уровень 0..1. */
    virtual float get() const = 0;
};