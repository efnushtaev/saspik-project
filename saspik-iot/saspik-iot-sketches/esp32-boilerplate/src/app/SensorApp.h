// SPDX-License-Identifier: MIT
/*
 * SensorApp.h — Composition Root шаблона «Sensor».
 *
 * Минимальный граф батарейного сенсора: портал (только при первом запуске) →
 * конфиг → лог (UART + ESP-NOW через шлюз) → ESP-NOW → шины/датчики →
 * light sleep. MQTT/WiFi STA в рабочем цикле не используются: данные уходят
 * шлюзу по ESP-NOW, а шлюз публикует их в MQTT (см. EspNowMqttBridge).
 *
 * Цикл работы: проснулись → читаем датчики по SENSOR_INTERVAL_MS → по
 * истечении SLEEP_INTERVAL_MS уходим в light sleep (millis() продолжает идти,
 * см. LightSleepManager). Авто-ребут по таймаутам WiFi/MQTT на сенсоре
 * намеренно не применяется — устройство спит (см. TODO.md).
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include "app/IApp.h"
#include "config/ConfigManager.h"
#include "config/NvsConfigStore.h"
#include "core/DeviceInfo.h"
#include "core/RuntimeConfig.h"
#include "core/TimerRegistry.h"
#include "app/examples/ChannelBank.h"
#include "espnow/EspNowManager.h"
#include "interfaces/bus/InterfaceManager.h"
#include "log/EspNowLogSink.h"
#include "log/Logger.h"
#include "log/RebootCauseStore.h"
#include "log/UartLogSink.h"
#include "portal/CaptivePortal.h"
#include "sys/LightSleepManager.h"

/**
 * @brief Шаблон «Sensor»: ESP-NOW + light sleep + шины/датчики.
 */
class SensorApp : public IApp {
public:
    SensorApp();

    bool begin() override;
    void update() override;

private:
    static void onSensorTimer(void* arg);
    static void onSleepCheckTimer(void* arg);
    void readAndSendSensors();
    void checkSleep();
    DeviceConfig buildDefaults() const;

    // Порядок объявления = порядок инициализации (важно для DI-связей).
    DeviceInfo m_info;
    RebootCauseStore m_rebootCause;
    NvsConfigStore m_configStore;
    ConfigManager m_configManager;
    CaptivePortal m_portal;
    Logger m_logger;
    UartLogSink m_uartSink;
    EspNowManager m_espNow;
    EspNowLogSink m_espNowSink;
    LightSleepManager m_sleep;
    TimerRegistry m_timers;
    RuntimeConfig m_cfg;

    ChannelBank m_channels;
    InterfaceManager m_interfaces;
};

#endif  // ARDUINO