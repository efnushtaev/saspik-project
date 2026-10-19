// SPDX-License-Identifier: MIT
/*
 * IMqttClient.h — контракт MQTT-клиента.
 *
 * Абстракция над PubSubClient: мост, лог-синк и команды работают с
 * интерфейсом, а host-тесты подставляют MockMqttClient. Публикация/подписка
 * принимают готовые C-строки (JSON собирается вызывающим).
 */

#pragma once

#include "mqtt/MqttConfig.h"
#include "net/IMessageHandler.h"

/**
 * @brief Минимальный MQTT-клиент (connect/sub/pub/dispatch).
 */
class IMqttClient {
public:
    virtual ~IMqttClient() = default;

    /**
     * @brief Настройка клиента (не подключается — подключение в update()).
     */
    virtual bool begin(const MqttConfig& cfg) = 0;

    /**
     * @brief Неблокирующий тик: поддержание соединения и чтение входящих.
     */
    virtual void update() = 0;

    /** @brief true, если соединение с брокером живо. */
    virtual bool isConnected() const = 0;

    /**
     * @brief Публикация сообщения.
     * @param retained true для retained (статусы online/offline)
     * @return false, если соединения нет или публикация не удалась.
     */
    virtual bool publish(const char* topic, const char* payload,
                         bool retained) = 0;

    /**
     * @brief Подписка на топик с диспетчеризацией в обработчик.
     */
    virtual bool subscribe(const char* topic, IMessageHandler* handler) = 0;
};