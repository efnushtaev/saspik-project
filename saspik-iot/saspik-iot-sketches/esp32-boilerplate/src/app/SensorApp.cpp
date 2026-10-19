// SPDX-License-Identifier: MIT
/*
 * SensorApp.cpp — реализация Composition Root сенсора.
 *
 * Ключевые решения:
 *  - лог уходит шлюзу по ESP-NOW (EspNowLogSink), а не в MQTT — у сенсора
 *    нет постоянного STA-соединения (энергосбережение, см. TODO.md);
 *  - перед сном обязателен flush логов: конверты в очередях после входа
 *    в сон будут потеряны (см. ISleepManager::enterSleep);
 *  - в режиме портала сон заблокирован (setBlocked) — иначе устройство
 *    уснёт, не дав пользователю закончить настройку.
 */

#include "app/SensorApp.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <string.h>

#include "espnow/EspNowCodec.h"

namespace {
// millis() возвращает unsigned long — отдельный тип от uint32_t (ClockFn);
// адаптер приводит к контракту TimerRegistry, сохраняя 32-битную арифметику.
uint32_t clockMillis() { return static_cast<uint32_t>(millis()); }
}  // namespace

SensorApp::SensorApp()
    : m_info(UNIT_ID, OBJECT_ID),
      m_rebootCause(),
      m_configStore(),
      m_configManager(m_configStore),
      m_portal(),
      m_logger(m_info, m_rebootCause),
      m_uartSink(Serial),
      m_espNow(&m_logger),
      // v1: единственный шлюз — DEFAULT_GATEWAY_MAC (см. config.h);
      // runtime-управление списком пиров — TODO (см. TODO.md).
      m_espNowSink(m_espNow, DEFAULT_GATEWAY_MAC),
      m_sleep(&m_logger),
      m_timers(&clockMillis),
      m_cfg(),
#if USE_EXAMPLE_DHT22
      m_dht22(PIN_GPIO_3),
#endif
#if USE_EXAMPLE_FLOAT
      m_floatSensor(PIN_GPIO_4),
#endif
      m_interfaces()  // последний инициализатор — всегда присутствует
{
}

bool SensorApp::begin() {
    m_rebootCause.begin();
    m_logger.addSink(&m_uartSink);
    m_logger.addSink(&m_espNowSink);

    const DeviceConfig defaults = buildDefaults();
    if (!m_portal.begin(m_cfg.wifi, PIN_GPIO_2, &defaults)) {
        // Портал активен: сон запрещён, пока пользователь настраивает сеть.
        m_sleep.setBlocked(true);
        m_logger.logInfo("portal", "captive portal active");
        return true;
    }
    m_cfg.app = m_configManager.load();

    m_espNow.begin(m_cfg.app.espNowChannel);
    m_espNow.addPeer(m_cfg.app.peers[0].mac);

    m_interfaces.begin();
#if USE_EXAMPLE_DHT22
    m_dht22.begin();
#endif
#if USE_EXAMPLE_FLOAT
    m_floatSensor.begin();
#endif

    m_sleep.configure(m_cfg.app.sleepIntervalMs);
    m_timers.add(m_cfg.app.sensorIntervalMs, onSensorTimer, this);
    m_timers.add(WAKE_CHECK_INTERVAL_MS, onSleepCheckTimer, this);

    m_logger.logInfo("app", "sensor started");
    return true;
}

void SensorApp::update() {
    m_portal.handlePortal();
    if (m_portal.isPortalMode()) {
        return;  // в режиме портала остальной граф не инициализирован
    }
    m_espNow.update();
    m_logger.update();
    m_timers.update();
}

void SensorApp::onSensorTimer(void* arg) {
    static_cast<SensorApp*>(arg)->readAndSendSensors();
}

void SensorApp::onSleepCheckTimer(void* arg) {
    static_cast<SensorApp*>(arg)->checkSleep();
}

void SensorApp::readAndSendSensors() {
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

void SensorApp::addSampleToPacket(EspNowPacket& packet,
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

void SensorApp::checkSleep() {
    if (!m_sleep.isSleepDue(millis())) {
        return;
    }
    // Перед сном выталкиваем очереди логов: конверты, оставшиеся в буферах,
    // после входа в сон будут потеряны (см. ISleepManager::enterSleep).
    m_logger.flush();
    m_sleep.enterSleep();
}

DeviceConfig SensorApp::buildDefaults() const {
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