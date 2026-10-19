// SPDX-License-Identifier: MIT
/*
 * DeviceApp.h — Composition Root шаблона «Device».
 *
 * Устройство без сна: портал → конфиг → лог (UART + ESP-NOW через шлюз) →
 * ESP-NOW rx/tx → шины/датчики/исполнители. Данные датчиков уходят шлюзу
 * пакетами DATA, команды от шлюза приходят пакетами COMMAND (поле
 * ESP_NOW_FIELD_COMMAND_JSON, формат {"state":"ON"|"OFF"} — см. mqtt-topics.md).
 *
 * v1: один исполнитель на объект — команда применяется ко всем подключённым
 * исполнителям; маршрутизация по имени (несколько исполнителей на объект) —
 * TODO (см. TODO.md).
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
#include "espnow/EspNowManager.h"
#include "examples/Dht22Sensor.h"
#include "examples/FloatSensor.h"
#include "examples/RelayActuator.h"
#include "interfaces/actuator/IActuator.h"
#include "interfaces/bus/InterfaceManager.h"
#include "interfaces/sensor/ISensor.h"
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
    void applyCommand(const char* payload);
    void publishDiag();
    void addSampleToPacket(EspNowPacket& packet, const SensorSample& sample);
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

#if USE_EXAMPLE_DHT22
    Dht22Sensor m_dht22;
#endif
#if USE_EXAMPLE_FLOAT
    FloatSensor m_floatSensor;
#endif
#if USE_EXAMPLE_RELAY
    RelayActuator m_relay;
#endif
    // m_interfaces объявлен ПОСЛЕ датчиков/исполнителей: в списке инициализации
    // он идёт последним (всегда присутствует) — иначе при выключенных флагах
    // примеров список заканчивался бы запятой (синтаксическая ошибка).
    InterfaceManager m_interfaces;
};

#endif  // ARDUINO