// SPDX-License-Identifier: MIT
/*
 * UartBus.h — обёртка аппаратного UART (Serial1/Serial2).
 *
 * Номер контроллера задаётся в config.h (UART_NUM: 1 или 2) — Serial0
 * зарезервирован под лог/загрузку, прикладной UART его не трогает.
 */

#pragma once

#include <stdint.h>

#include "interfaces/bus/IBus.h"

#ifdef ARDUINO
#include <HardwareSerial.h>
#endif

/**
 * @brief Прикладной UART.
 */
class UartBus : public IBus {
public:
    /**
     * @param num   номер контроллера (UART_NUM: 1 = Serial1, 2 = Serial2)
     * @param baud  скорость (UART_BAUD)
     * @param txPin пин TX (PIN_UART_TX)
     * @param rxPin пин RX (PIN_UART_RX)
     */
    UartBus(uint8_t num, uint32_t baud, uint8_t txPin, uint8_t rxPin);

    bool begin() override;
    bool isReady() const override;

#ifdef ARDUINO
    /** @brief Хэндл HardwareSerial для драйверов устройств. */
    HardwareSerial& serial();
#endif

private:
    uint8_t m_num;
    uint32_t m_baud;
    uint8_t m_txPin;
    uint8_t m_rxPin;
    bool m_ready;

#ifdef ARDUINO
    HardwareSerial* m_serial;
#endif
};