// SPDX-License-Identifier: MIT
/*
 * UartBus.cpp — реализация обёртки UART.
 */

#include "interfaces/bus/UartBus.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <soc/soc_caps.h>

UartBus::UartBus(uint8_t num, uint32_t baud, uint8_t txPin, uint8_t rxPin)
    : m_num(num),
      m_baud(baud),
      m_txPin(txPin),
      m_rxPin(rxPin),
      m_ready(false),
      m_serial(nullptr) {}

bool UartBus::begin() {
    if (m_ready) {
        return true;
    }
    // Только Serial1/Serial2: Serial0 занят логом и прошивкой.
    // Serial2 существует не на всех чипах (S2/C3 имеют 2 UART) — выбор
    // по числу контроллеров SoC, иначе сборка падает на отсутствии символа.
#if SOC_UART_NUM > 2
    m_serial = (m_num == 2) ? &Serial2 : &Serial1;
#else
    m_serial = &Serial1;
#endif
    // Формат SERIAL_8N1 фиксируем в коде: для v1 прикладных протоколов
    // хватает; параметры кадра в config.h — при необходимости (TODO).
    m_serial->begin(m_baud, SERIAL_8N1, m_rxPin, m_txPin);
    m_ready = true;
    return true;
}

bool UartBus::isReady() const { return m_ready; }

HardwareSerial& UartBus::serial() { return *m_serial; }

#endif  // ARDUINO