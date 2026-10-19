// SPDX-License-Identifier: MIT
/*
 * EspNowManager.h — реализация IEspNowClient поверх esp_now API.
 *
 * Приём двухфазный: аппаратный колбэк (контекст WiFi-задачи) только копирует
 * сырые байты в m_rxBuffer, а декодирование и вызов слушателя происходят в
 * update() (контекст loop()). Это принципиально: кодек и бизнес-логика не
 * должны выполняться в контексте задачи WiFi/TCPIP.
 *
 * Статический s_instance — то же исключение из «без глобалов», что у
 * MqttManager: esp_now регистрирует C-колбэки без user-контекста.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "espnow/IEspNowClient.h"

class ILogger;

/**
 * @brief ESP-NOW транспорт: WiFi STA + esp_now + однослотовый RX-буфер.
 */
class EspNowManager : public IEspNowClient {
public:
    /**
     * @param logger опциональный логгер (nullptr разрешён)
     */
    explicit EspNowManager(ILogger* logger);

    bool begin(uint8_t channel) override;
    bool addPeer(const uint8_t* mac) override;
    void update() override;
    bool send(const uint8_t* mac, const uint8_t* data, size_t len) override;
    void setListener(IEspNowListener* listener) override;

private:
    static void onRecvCb(const uint8_t* mac, const uint8_t* data, int len);
    void dispatchPending();

    static EspNowManager* s_instance;  // только для esp_now колбэков

    ILogger* m_logger;
    IEspNowListener* m_listener;
    bool m_ready;
    uint8_t m_peerCount;  // зарегистрировано пиров (лимит ESP_NOW_MAX_PEERS)
    uint8_t m_rxMac[6];
    uint8_t m_rxBuffer[ESP_NOW_PACKET_MAX];
    size_t m_rxLen;
    bool m_rxPending;
};