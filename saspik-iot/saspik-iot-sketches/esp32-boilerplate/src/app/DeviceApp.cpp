// SPDX-License-Identifier: MIT
/*
 * DeviceApp.cpp — реализация Composition Root устройства.
 *
 * В отличие от SensorApp здесь нет сна и есть приём команд: устройство
 * слушает ESP-NOW (setListener(this)) и применяет COMMAND-пакеты шлюза
 * к своим исполнителям. Данные датчиков уходят шлюзу так же, как у сенсора.
 */

#include "app/DeviceApp.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <ArduinoJson.h>
#include <string.h>

#include "espnow/EspNowCodec.h"

namespace {
// millis() возвращает unsigned long — отдельный тип от uint32_t (ClockFn);
// адаптер приводит к контракту TimerRegistry, сохраняя 32-битную арифметику.
uint32_t clockMillis() { return static_cast<uint32_t>(millis()); }
}  // namespace

DeviceApp::DeviceApp()
    : m_info(UNIT_ID, OBJECT_ID),
      m_rebootCause(),
      m_configStore(),
      m_configManager(m_configStore),
      m_portal(),
      m_logger(m_info, m_rebootCause),
      m_uartSink(Serial),
      m_espNow(&m_logger),
      // v1: единственный шлюз — DEFAULT_GATEWAY_MAC (см. config.h).
      m_espNowSink(m_espNow, DEFAULT_GATEWAY_MAC),
      m_timers(&clockMillis),
      m_cfg(),
#if USE_EXAMPLE_DHT22
      m_dht22(PIN_GPIO_3),
#endif
#if USE_EXAMPLE_FLOAT
      m_floatSensor(PIN_GPIO_4),
#endif
#if USE_EXAMPLE_RELAY
      m_relay(PIN_GPIO_5),
#endif
      m_interfaces()  // последний инициализатор — всегда присутствует
{
}

bool DeviceApp::begin() {
    m_rebootCause.begin();
    m_logger.addSink(&m_uartSink);
    m_logger.addSink(&m_espNowSink);

    const DeviceConfig defaults = buildDefaults();
    if (!m_portal.begin(m_cfg.wifi, PIN_GPIO_2, &defaults)) {
        // Портал активен: остальной граф поднимется после рестарта от /save.
        m_logger.logInfo("portal", "captive portal active");
        return true;
    }
    m_cfg.app = m_configManager.load();

    m_espNow.begin(m_cfg.app.espNowChannel);
    m_espNow.addPeer(m_cfg.app.peers[0].mac);
    m_espNow.setListener(this);  // устройство принимает команды шлюза

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

    m_timers.add(m_cfg.app.sensorIntervalMs, onSensorTimer, this);
    m_timers.add(m_cfg.app.diagIntervalMs, onDiagTimer, this);

    m_logger.logInfo("app", "device started");
    return true;
}

void DeviceApp::update() {
    m_portal.handlePortal();
    if (m_portal.isPortalMode()) {
        return;  // в режиме портала остальной граф не инициализирован
    }
    m_espNow.update();
    m_logger.update();
    m_timers.update();
}

void DeviceApp::onEspNowPacket(const uint8_t* srcMac,
                               const EspNowPacket& packet) {
    // v1: принимаем команды от любого пира (список пиров фиксирован —
    // DEFAULT_GATEWAY_MAC); проверка srcMac — TODO при расширении списка.
    (void)srcMac;
    if (packet.type != ESP_NOW_TYPE_COMMAND) {
        return;  // данные/логи/ACK устройству не адресованы
    }
    for (uint8_t i = 0; i < packet.fieldCount; ++i) {
        const EspNowField& field = packet.fields[i];
        if (field.id != ESP_NOW_FIELD_COMMAND_JSON || field.len == 0) {
            continue;
        }
        // JSON в кадре не null-terminated — копируем с ограничением.
        char payload[LOG_ENVELOPE_MAX_LEN];
        const size_t copyLen = field.len < sizeof(payload) - 1
                                   ? field.len
                                   : sizeof(payload) - 1;
        memcpy(payload, field.data, copyLen);
        payload[copyLen] = '\0';
        applyCommand(payload);
        return;
    }
}

void DeviceApp::onSensorTimer(void* arg) {
    static_cast<DeviceApp*>(arg)->readAndSendSensors();
}

void DeviceApp::onDiagTimer(void* arg) {
    static_cast<DeviceApp*>(arg)->publishDiag();
}

void DeviceApp::readAndSendSensors() {
    EspNowPacket packet = {};
    packet.type = ESP_NOW_TYPE_DATA;
    packet.timestampMs = millis();

#if USE_EXAMPLE_DHT22
    addSampleToPacket(packet, m_dht22.read());
#endif
#if USE_EXAMPLE_FLOAT
    addSampleToPacket(packet, m_floatSensor.read());
#endif

    if (packet.fieldCount == 0) {
        return;  // ни одного валидного измерения — эфир не мусорим
    }
    uint8_t buffer[ESP_NOW_PACKET_MAX];
    const size_t len = EspNowCodec::encode(packet, buffer, sizeof(buffer));
    if (len > 0) {
        m_espNow.send(m_cfg.app.peers[0].mac, buffer, len);
    }
}

void DeviceApp::addSampleToPacket(EspNowPacket& packet,
                                  const SensorSample& sample) {
    if (!sample.valid || packet.fieldCount >= ESP_NOW_MAX_FIELDS) {
        return;  // невалидное измерение или пакет уже полон
    }
    EspNowField& field = packet.fields[packet.fieldCount++];
    field.id = sample.fieldId;
    // Кодек копирует данные в буфер сразу (encode), поэтому указатель на
    // локальную переменную безопасен.
    field.data = reinterpret_cast<const uint8_t*>(&sample.value);
    field.len = sizeof(sample.value);
}

void DeviceApp::applyCommand(const char* payload) {
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

void DeviceApp::publishDiag() {
    char msg[LOG_MSG_MAX_LEN];
    snprintf(msg, sizeof(msg), "espnow ready, peers=%d",
             static_cast<int>(m_cfg.app.peerCount));
    m_logger.logInfo("diag", msg);
}

DeviceConfig DeviceApp::buildDefaults() const {
    // Дефолты формы портала — из config.h (см. GatewayApp::buildDefaults).
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