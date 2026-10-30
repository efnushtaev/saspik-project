// SPDX-License-Identifier: MIT
/*
 * DeviceApp.h — Composition Root шаблона «Device».
 *
 * Устройство без сна: портал → конфиг → лог (UART + ESP-NOW через шлюз) →
 * ESP-NOW rx/tx → шины/датчики/исполнители. Каналы собираются из CHANNELS[]
 * (config.h). Данные уходят шлюзу пакетами DATA с JSON-картой имён, команды
 * приходят пакетами COMMAND ({"channel":"<имя>","value":"ON"|"OFF"}), а
 * состояние автоматов — пакетами STATE (retained в MQTT публикует мост).
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

/**
 * @brief Шаблон «Device»: ESP-NOW rx/tx + шины/датчики/исполнители.
 */
class DeviceApp : public IApp, public IEspNowListener {
public:
    DeviceApp();

    bool begin() override;
    void update() override;

    // IEspNowListener: приём команд от шлюза (пакеты COMMAND).
    void onEspNowPacket(const uint8_t* srcMac,
                        const EspNowPacket& packet) override;

private:
    static void onSensorTimer(void* arg);
    static void onDiagTimer(void* arg);
    void readAndSendSensors();
    void sendState();
    void applyCommand(const char* payload);
    void publishDiag();
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
    TimerRegistry m_timers;
    RuntimeConfig m_cfg;

    ChannelBank m_channels;
    InterfaceManager m_interfaces;
};

#endif  // ARDUINO