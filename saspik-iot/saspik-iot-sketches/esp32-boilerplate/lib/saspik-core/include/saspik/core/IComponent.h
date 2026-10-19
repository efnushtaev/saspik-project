// SPDX-License-Identifier: MIT
/*
 * IComponent.h — базовый жизненный цикл всех менеджеров boilerplate.
 *
 * Почему интерфейс: любой менеджер (WifiManager, MqttManager, EspNowManager...)
 * обязан пройти begin() (инициализация с возможным отказом) и update()
 * (неблокирующий тик из loop). Единый контракт позволяет App-шаблонам
 * обращаться с модулями единообразно и не знать конкретные классы.
 */

#pragma once

/**
 * @brief Общий жизненный цикл компонента (SRP: только «запустить» и «тикнуть»).
 */
class IComponent {
public:
    virtual ~IComponent() = default;

    /**
     * @brief Инициализация компонента.
     * @return true при успехе; false — компонент не готов к работе.
     */
    virtual bool begin() = 0;

    /**
     * @brief Неблокирующий тик. Вызывается каждый цикл loop().
     */
    virtual void update() = 0;
};