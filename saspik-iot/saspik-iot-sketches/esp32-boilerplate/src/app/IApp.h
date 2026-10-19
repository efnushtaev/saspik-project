// SPDX-License-Identifier: MIT
/*
 * IApp.h — контракт шаблона приложения (Composition Root).
 *
 * Каждый шаблон (Gateway/Sensor/Device) — это отдельный класс, который
 * собирает граф объектов через конструкторы (DI) и задаёт порядок begin().
 * main.cpp не знает про конкретные шаблоны: он выбирает IApp по DEVICE_ROLE
 * и вызывает ровно два метода. Заголовок чистый (без Arduino) — контракт
 * не зависит от платформы.
 */

#pragma once

/**
 * @brief Жизненный цикл шаблона приложения.
 */
class IApp {
public:
    virtual ~IApp() = default;

    /**
     * @brief Инициализация всего графа объектов (порядок begin — внутри App).
     * @return true при успехе; false — шаблон не готов к работе.
     */
    virtual bool begin() = 0;

    /**
     * @brief Неблокирующий тик из loop().
     */
    virtual void update() = 0;
};