// SPDX-License-Identifier: MIT
/*
 * IEspNowClient.h — контракт ESP-NOW транспорта.
 *
 * Клиент работает с СЫРЫМИ байтами: фрейминг (EspNowCodec) и семантика
 * (EspNowMqttBridge) вынесены в отдельные модули (SRP). На приёме кодек
 * применяется внутри EspNowManager перед вызовом слушателя, на передаче —
 * вызывающим (мост/приложение). Host-тесты подставляют MockEspNowClient.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "espnow/EspNowTypes.h"

/** @brief Слушатель входящих ESP-NOW пакетов (уже декодированных). */
class IEspNowListener {
public:
    virtual ~IEspNowListener() = default;

    /**
     * @param srcMac MAC отправителя (6 байт)
     * @param packet декодированный пакет; поля валидны только на время вызова
     */
    virtual void onEspNowPacket(const uint8_t* srcMac,
                                const EspNowPacket& packet) = 0;
};

/** @brief Контракт ESP-NOW клиента. */
class IEspNowClient {
public:
    virtual ~IEspNowClient() = default;

    /**
     * @brief Инициализация WiFi (STA) и esp_now на заданном канале.
     */
    virtual bool begin(uint8_t channel) = 0;

    /**
     * @brief Регистрация пира (до ESP_NOW_MAX_PEERS).
     */
    virtual bool addPeer(const uint8_t* mac) = 0;

    /** @brief Неблокирующий тик: доставка принятых пакетов слушателю. */
    virtual void update() = 0;

    /**
     * @brief Отправка готовых байт пакета пиру.
     * @return false: не инициализирован / нет пира / буфер слишком велик.
     */
    virtual bool send(const uint8_t* mac, const uint8_t* data, size_t len) = 0;

    /** @brief Установить слушателя входящих (один на менеджер). */
    virtual void setListener(IEspNowListener* listener) = 0;
};