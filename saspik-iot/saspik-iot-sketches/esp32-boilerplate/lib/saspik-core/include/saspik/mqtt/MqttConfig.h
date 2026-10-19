// SPDX-License-Identifier: MIT
/*
 * MqttConfig.h — параметры подключения к брокеру.
 *
 * Чистый value type: значения приходят из RuntimeConfig (портал/NVS) и
 * compile-time констант config.h. Отдельный тип позволяет MqttManager не
 * зависеть от DeviceConfig (shared/wifi-config) и тестироваться на хосте.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Параметры MQTT-подключения.
 */
struct MqttConfig {
    const char* host;             ///< Адрес брокера
    uint16_t port;                ///< Порт (1883)
    const char* user;             ///< Логин ("" — без авторизации)
    const char* pass;             ///< Пароль
    const char* statusTopic;      ///< device/{unitId}/{objectId}/status (LWT+online)
    // Уникальный clientId (pub/sub соединение). App собирает его как
    // CLIENT_ID_PREFIX + OBJECT_ID — см. config.h, раздел «Идентификация».
    const char* clientId;
    uint32_t connectTimeoutMs;    ///< Таймаут TCP-connect (против Task WDT)
    uint32_t reconnectIntervalMs; ///< Пауза между попытками connect
    uint32_t rebootTimeoutMs;     ///< Недоступность дольше → авто-ребут
    bool enableRebootWatchdog;    ///< false для Sensor (спит)
};