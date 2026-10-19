// SPDX-License-Identifier: MIT
/*
 * GatewayApp.h — Composition Root шаблона «Gateway».
 *
 * Собирает полный граф шлюза: captive portal → конфиг → лог (UART+MQTT) →
 * WiFi → MQTT → ESP-NOW → мост ESP-NOW↔MQTT → свои датчики/исполнители
 * (опционально, флаги USE_EXAMPLE_*). Порядок begin() важен: ESP-NOW требует
 * поднятого WiFi-радио, мост — живых MQTT/ESP-NOW клиентов.
 *
 * Свои данные шлюз публикует в sensor/{unitId}/{objectIdШлюза} напрямую в MQTT
 * (в отличие от узлов, чьи данные приходят по ESP-NOW и публикует мост).
 * Свои команды ({"state":"ON"|"OFF"}, см. mqtt-topics.md) шлюз принимает
 * на device/{unitId}/{objectIdШлюза} — для этого App реализует IMessageHandler.
 *
 * Файл firmware-only: обёрнут в #ifdef ARDUINO (на хосте — пустой заголовок).
 */

#pragma once

#ifdef ARDUINO

#include "app/IApp.h"
#include "bridge/EspNowMqttBridge.h"
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
#include "log/Logger.h"
#include "log/MqttLogSink.h"
#include "log/RebootCauseStore.h"
#include "log/UartLogSink.h"
#include "mqtt/MqttManager.h"
#include "net/IMessageHandler.h"
#include "portal/CaptivePortal.h"
#include "wifi/WifiManager.h"

/**
 * @brief Шаблон «Gateway»: WiFi + MQTT + ESP-NOW мост (+ свои периферия).
 */
class GatewayApp : public IApp, public IMessageHandler {
public:
    GatewayApp();

    bool begin() override;
    void update() override;

    // IMessageHandler: команды на собственные исполнители шлюза
    // (device/{unitId}/{objectIdШлюза}, формат {"state":"ON"|"OFF"}).
    void onMessage(const char* topic, const char* payload) override;

private:
    static void onSensorTimer(void* arg);
    static void onDiagTimer(void* arg);
    void publishOwnSensors();
    void publishDiag();
    void applyOwnCommand(const char* payload);
    void attachPeripherals();
    MqttConfig buildMqttConfig();
    DeviceConfig buildDefaults() const;

    // Порядок объявления = порядок инициализации (важно для DI-связей:
    // Logger требует m_info/m_rebootCause, sink'и — m_mqtt/m_espNow и т.д.).
    DeviceInfo m_info;
    RebootCauseStore m_rebootCause;
    NvsConfigStore m_configStore;
    ConfigManager m_configManager;
    CaptivePortal m_portal;
    Logger m_logger;
    UartLogSink m_uartSink;
    WifiManager m_wifi;
    MqttManager m_mqtt;
    MqttLogSink m_mqttSink;
    EspNowManager m_espNow;
    EspNowMqttBridge m_bridge;
    TimerRegistry m_timers;
    RuntimeConfig m_cfg;
    InterfaceManager m_interfaces;

#if USE_EXAMPLE_DHT22
    Dht22Sensor m_dht22;
#endif
#if USE_EXAMPLE_FLOAT
    FloatSensor m_floatSensor;
#endif
#if USE_EXAMPLE_RELAY
    RelayActuator m_relay;
#endif

    // Буферы MqttConfig: MqttConfig хранит const char* — строки должны жить
    // столько же, сколько живёт конфиг (весь цикл работы приложения).
    char m_clientId[32];
    char m_statusTopic[MQTT_TOPIC_MAX_LEN];
    bool m_startupLogSent;
};

#endif  // ARDUINO