// SPDX-License-Identifier: MIT
/*
 * IMessageHandler.h — обработчик одного входящего MQTT-сообщения.
 *
 * MqttManager диспетчеризует сообщения по топикам зарегистрированным
 * обработчикам; сами обработчики (команды устройства, мост ESP-NOW↔MQTT)
 * реализуют этот интерфейс. Так MqttManager не знает про конкретные команды.
 */

#pragma once

/**
 * @brief Обработчик входящего MQTT-сообщения.
 */
class IMessageHandler {
public:
    virtual ~IMessageHandler() = default;

    /**
     * @param topic   топик, на который подписан обработчик
     * @param payload тело сообщения (JSON), null-terminated
     */
    virtual void onMessage(const char* topic, const char* payload) = 0;
};