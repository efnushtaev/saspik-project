// SPDX-License-Identifier: MIT
/*
 * InterfaceManager.h — владелец и инициализатор шин из config.h.
 *
 * Единственное место, где знание «какие шины включены и на каких пинах»
 * встречается в коде: флаги BUS_*_ENABLED — compile-time, поэтому
 * неиспользуемые шины не создаются вовсе (экономия RAM и пинов).
 * Приложение получает доступы через i2c()/oneWire()/spi()/uart() —
 * для выключенной шины возвращается nullptr, и пример/драйвер просто
 * не подключается.
 *
 * ARDUINO-гвард в заголовке: на хосте класс вырождается (шины —
 * железная тема, host-тестами не покрываются).
 */

#pragma once

#ifdef ARDUINO

#include "config.h"
#include "interfaces/bus/I2cBus.h"
#include "interfaces/bus/OneWireBus.h"
#include "interfaces/bus/SpiBus.h"
#include "interfaces/bus/UartBus.h"

/**
 * @brief Композиция шин по config.h.
 */
class InterfaceManager {
public:
    InterfaceManager();

    /** @brief Создать и инициализировать только включённые шины. */
    void begin();

    /** @brief I2C или nullptr, если BUS_I2C_ENABLED=false. */
    I2cBus* i2c() { return m_i2c; }

    /** @brief OneWire или nullptr, если BUS_ONE_WIRE_ENABLED=false. */
    OneWireBus* oneWire() { return m_oneWire; }

    /** @brief SPI или nullptr, если BUS_SPI_ENABLED=false. */
    SpiBus* spi() { return m_spi; }

    /** @brief UART или nullptr, если BUS_UART_ENABLED=false. */
    UartBus* uart() { return m_uart; }

private:
    I2cBus* m_i2c;
    OneWireBus* m_oneWire;
    SpiBus* m_spi;
    UartBus* m_uart;
};

#endif  // ARDUINO