// SPDX-License-Identifier: MIT
/*
 * GatewayApp.cpp — реализация Composition Root шлюза.
 *
 * begin() строго по порядку из архитектуры: портал → конфиг → лог → WiFi →
 * MQTT → ESP-NOW → мост → периферия → таймеры. Если портал активен, остальной
 * граф не инициализируется: после POST /save библиотека сама делает
 * ESP.restart(), и устройство стартует уже в штатном режиме.
 */

#include "app/GatewayApp.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "espnow/EspNowCodec.h"
#include "net/TopicBuilder.h"

namespace {
// millis() возвращает unsigned long — отдельный тип от uint32_t (ClockFn);
// адаптер приводит к контракту TimerRegistry, сохраняя 32-битную арифметику.
uint32_t clockMillis() { return static_cast<uint32_t>(millis()); }

// Добавляет валидное измерение в JSON-документ: fieldId → имя ключа
// по FIELD_DICTIONARY (config.h). Невалидные измерения пропускаются.
void addSampleToJson(StaticJsonDocument<MQTT_PAYLOAD_MAX>& doc,
                     const SensorSample& sample) {
    if (!sample.valid) {
        return;
    }
    for (size_t i = 0; i < FIELD_DICTIONARY_SIZE; ++i) {
        if (FIELD_DICTIONARY[i].id == sample.fieldId) {
            doc[FIELD_DICTIONARY[i].key] = sample.value;
            return;
        }
    }
}
}  // namespace

GatewayApp::GatewayApp()
    : m_info(UNIT_ID, OBJECT_ID),
      m_rebootCause(),
      m_configStore(),
      m_configManager(m_configStore),
      m_portal(),
      m_logger(m_info, m_rebootCause),
      m_uartSink(Serial),
      m_wifi(&m_logger, WIFI_RECONNECT_INTERVAL_MS, WIFI_REBOOT_TIMEOUT_MS,
             true),
      m_mqtt(&m_logger),
      m_mqttSink(m_mqtt, m_info.unitId(), m_info.objectId()),
      m_espNow(&m_logger),
      m_bridge(&m_mqtt, &m_espNow, &m_logger, m_info),
      m_timers(&clockMillis),
      m_cfg(),
      m_interfaces(),
#if USE_EXAMPLE_DHT22
      m_dht22(PIN_GPIO_3),
#endif
#if USE_EXAMPLE_FLOAT
      m_floatSensor(PIN_GPIO_4),
#endif
#if USE_EXAMPLE_RELAY
      m_relay(PIN_GPIO_5),
#endif
      m_startupLogSent(false) {
    m_clientId[0] = '\0';
    m_statusTopic[0] = '\0';
}

bool GatewayApp::begin() {
    m_rebootCause.begin();
    m_logger.addSink(&m_uartSink);
    m_logger.addSink(&m_mqttSink);

    const DeviceConfig defaults = buildDefaults();
    if (!m_portal.begin(m_cfg.wifi, PIN_GPIO_2, &defaults)) {
        // Портал активен: остальной граф поднимется после рестарта от /save.
        m_logger.logInfo("portal", "captive portal active");
        return true;
    }
    m_cfg.app = m_configManager.load();

    m_wifi.begin(m_cfg.wifi.wifiSsid, m_cfg.wifi.wifiPass);
    m_mqtt.begin(buildMqttConfig());
    m_espNow.begin(m_cfg.app.espNowChannel);
    m_bridge.begin();

    // Команды узлам: подписка на device/{unitId}/+ маршрутизируется мостом
    // в ESP-NOW. Свои команды — точная подписка на device/{unitId}/{objectId}
    // (MqttManager отдаёт приоритет точному совпадению, см. findSubscription).
    char wildcard[MQTT_TOPIC_MAX_LEN];
    TopicBuilder::commandWildcardTopic(wildcard, sizeof(wildcard),
                                       m_info.unitId());
    m_mqtt.subscribe(wildcard, &m_bridge);
    char ownCommand[MQTT_TOPIC_MAX_LEN];
    TopicBuilder::commandTopic(ownCommand, sizeof(ownCommand), m_info.unitId(),
                               m_info.objectId());
    m_mqtt.subscribe(ownCommand, this);

    attachPeripherals();

    m_timers.add(m_cfg.app.sensorIntervalMs, onSensorTimer, this);
    m_timers.add(m_cfg.app.diagIntervalMs, onDiagTimer, this);

    m_logger.logInfo("app", "gateway started");
    return true;
}

void GatewayApp::update() {
    m_portal.handlePortal();
    if (m_portal.isPortalMode()) {
        return;  // в режиме портала остальной граф не инициализирован
    }
    m_wifi.update();
    m_mqtt.update();
    m_espNow.update();
    m_logger.update();
    m_timers.update();

    // Стартовый конверт с причиной ребута — один раз после первого
    // успешного подключения (поведение publishStartupLog из esp32-local-mqtt).
    if (!m_startupLogSent && m_mqtt.isConnected()) {
        m_logger.publishStartup();
        m_startupLogSent = true;
    }
}

void GatewayApp::onMessage(const char* topic, const char* payload) {
    // Своя команда: топик уже отфильтрован MqttManager (точная подписка).
    (void)topic;
    applyOwnCommand(payload);
}

void GatewayApp::onSensorTimer(void* arg) {
    static_cast<GatewayApp*>(arg)->publishOwnSensors();
}

void GatewayApp::onDiagTimer(void* arg) {
    static_cast<GatewayApp*>(arg)->publishDiag();
}

void GatewayApp::publishOwnSensors() {
    if (!m_mqtt.isConnected()) {
        return;  // публиковать в никуда — только мусорить в лог
    }
    char topic[MQTT_TOPIC_MAX_LEN];
    TopicBuilder::dataTopic(topic, sizeof(topic), m_info.unitId(),
                            m_info.objectId());
    char payload[MQTT_PAYLOAD_MAX];
    StaticJsonDocument<MQTT_PAYLOAD_MAX> doc;

#if USE_EXAMPLE_DHT22
    addSampleToJson(doc, m_dht22.read());
#endif
#if USE_EXAMPLE_FLOAT
    addSampleToJson(doc, m_floatSensor.read());
#endif

    if (doc.isNull()) {
        return;  // ни одного валидного измерения — эфир/брокер не мусорим
    }
    serializeJson(doc, payload, sizeof(payload));
    m_mqtt.publish(topic, payload, false);
}

void GatewayApp::publishDiag() {
    char msg[LOG_MSG_MAX_LEN];
    snprintf(msg, sizeof(msg), "wifi=%d mqtt=%d rssi=%d",
             m_wifi.isConnected() ? 1 : 0, m_mqtt.isConnected() ? 1 : 0,
             m_wifi.rssi());
    m_logger.logInfo("diag", msg);
}

void GatewayApp::applyOwnCommand(const char* payload) {
    // Формат команды — из mqtt-topics.md: {"state":"ON"|"OFF"}.
    StaticJsonDocument<MQTT_PAYLOAD_MAX> doc;
    if (deserializeJson(doc, payload)) {
        m_logger.logWarn("cmd", "bad json");
        return;
    }
    const char* state = doc["state"];
    if (state == nullptr) {
        m_logger.logWarn("cmd", "no state field");
        return;
    }
    const float level = (strcmp(state, "ON") == 0) ? 1.0f : 0.0f;
#if USE_EXAMPLE_RELAY
    m_relay.set(level);
#endif
    m_logger.logInfo("cmd", state);
}

void GatewayApp::attachPeripherals() {
    m_interfaces.begin();
#if USE_EXAMPLE_DHT22
    m_dht22.begin();
#endif
#if USE_EXAMPLE_FLOAT
    m_floatSensor.begin();
#endif
#if USE_EXAMPLE_RELAY
    m_relay.begin();
#endif
}

MqttConfig GatewayApp::buildMqttConfig() {
    MqttConfig cfg = {};
    cfg.host = m_cfg.wifi.mqttHost;
    cfg.port = m_cfg.wifi.mqttPort;
    cfg.user = m_cfg.wifi.mqttUser;
    cfg.pass = m_cfg.wifi.mqttPass;
    TopicBuilder::statusTopic(m_statusTopic, sizeof(m_statusTopic),
                              m_info.unitId(), m_info.objectId());
    cfg.statusTopic = m_statusTopic;
    // Полный clientId собирается из префикса роли и objectId (см. config.h).
    snprintf(m_clientId, sizeof(m_clientId), "%s%s", CLIENT_ID_PREFIX,
             OBJECT_ID);
    cfg.clientId = m_clientId;
    cfg.connectTimeoutMs = MQTT_CONNECT_TIMEOUT_MS;
    cfg.reconnectIntervalMs = MQTT_RECONNECT_INTERVAL_MS;
    cfg.rebootTimeoutMs = MQTT_REBOOT_TIMEOUT_MS;
    cfg.enableRebootWatchdog = true;  // шлюз не спит — watchdog уместен
    return cfg;
}

DeviceConfig GatewayApp::buildDefaults() const {
    // Дефолты формы портала — из config.h (единственное место «заводских»
    // значений; реальные значения живут в NVS после первого /save).
    DeviceConfig d = {};
    strncpy(d.wifiSsid, WIFI_SSID, sizeof(d.wifiSsid) - 1);
    strncpy(d.wifiPass, WIFI_PASS, sizeof(d.wifiPass) - 1);
    strncpy(d.mqttHost, MQTT_BROKER, sizeof(d.mqttHost) - 1);
    d.mqttPort = MQTT_PORT;
    strncpy(d.mqttUser, MQTT_USER, sizeof(d.mqttUser) - 1);
    strncpy(d.mqttPass, MQTT_PASS, sizeof(d.mqttPass) - 1);
    return d;
}

#endif  // ARDUINO