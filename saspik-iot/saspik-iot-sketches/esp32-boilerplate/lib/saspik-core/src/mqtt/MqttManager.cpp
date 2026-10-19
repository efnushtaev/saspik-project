// SPDX-License-Identifier: MIT
/*
 * MqttManager.cpp — реализация на PubSubClient.
 *
 * Поведение перенесено из esp32-local-mqtt (connectMQTT / checkMqttTimeout):
 *   - попытки connect не чаще m_cfg.reconnectIntervalMs — иначе при
 *     недоступном брокере loop() блокируется на connect дольше Task WDT
 *     (известный анти-паттерн rst:0x8 из esp32-local-mqtt);
 *   - LWT "offline" выставляется в самом connect (последняя воля), после
 *     успешного connect публикуется retained "online";
 *   - при непрерывной недоступности дольше m_cfg.rebootTimeoutMs — полный
 *     рестарт (watchdog отключается для Sensor, см. enableRebootWatchdog).
 * Подписки хранятся в менеджере, потому что PubSubClient не восстанавливает
 * их после reconnect — переподписка вручную (resubscribeAll).
 */

#include "mqtt/MqttManager.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <string.h>

#include "log/ILogger.h"  // заголовок даёт только forward-declaration

namespace {
constexpr char LWT_ONLINE[] = "online";
constexpr char LWT_OFFLINE[] = "offline";
}  // namespace

// PIMPL-транспорт: единственное место с Arduino-типами менеджера.
struct MqttManager::Impl {
    WiFiClient wifi;
    PubSubClient mqtt{wifi};
};

MqttManager* MqttManager::s_instance = nullptr;

MqttManager::MqttManager(ILogger* logger)
    : m_logger(logger),
      m_cfg{},
      m_impl(nullptr),
      m_configured(false),
      m_lastConnectAttemptMs(0),
      m_brokerLostSinceMs(0),
      m_subscriptionCount(0) {
    for (auto& sub : m_subscriptions) {
        sub.handler = nullptr;
        sub.topic[0] = '\0';
    }
}

MqttManager::~MqttManager() {
    delete m_impl;
    m_impl = nullptr;
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool MqttManager::begin(const MqttConfig& cfg) {
    if (m_impl == nullptr) {
        m_impl = new Impl();
        if (m_impl == nullptr) {
            return false;  // heap исчерпан в setup — прошивка не стартует
        }
    }
    m_cfg = cfg;
    m_configured = true;
    s_instance = this;
    m_lastConnectAttemptMs = 0;
    m_brokerLostSinceMs = 0;

    m_impl->mqtt.setServer(cfg.host, cfg.port);
    m_impl->mqtt.setCallback(&MqttManager::onMessageCallback);
    // Буфер по максимуму исходящего (JSON-конверт лога LOG_ENVELOPE_MAX_LEN);
    // входящие команды (≤ MQTT_PAYLOAD_MAX) в него гарантированно влезают.
    m_impl->mqtt.setBufferSize(LOG_ENVELOPE_MAX_LEN);
    // Socket timeout (секунды) ограничивает блокировку на чтении/connect —
    // часть защиты от зависаний при недоступном брокере.
    const uint16_t sockTimeoutSec =
        static_cast<uint16_t>((m_cfg.connectTimeoutMs + 999) / 1000);
    m_impl->mqtt.setSocketTimeout(sockTimeoutSec < 1 ? 1 : sockTimeoutSec);
    return true;
}

void MqttManager::update() {
    if (!m_configured || m_impl == nullptr) {
        return;
    }
    const uint32_t now = millis();
    if (m_impl->mqtt.connected()) {
        m_brokerLostSinceMs = 0;
        m_impl->mqtt.loop();  // чтение входящих + поддержание keepalive
        return;
    }
    if (m_brokerLostSinceMs == 0) {
        m_brokerLostSinceMs = now;
    }
    tryConnect(now);
    tryRebootIfBrokerLost(now);
}

void MqttManager::tryConnect(uint32_t now) {
    if (now - m_lastConnectAttemptMs < m_cfg.reconnectIntervalMs) {
        return;
    }
    m_lastConnectAttemptMs = now;
    // Без WiFi connect падает сразу и только мусорит в лог — пропускаем.
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }
    const char* willTopic = m_cfg.statusTopic;
    // LWT: брокер сам опубликует "offline", если соединение оборвётся
    // внезапно (сбой питания, пропажа радиоканала) — подписчики статуса
    // не будут ждать keepalive-таймаута.
    const bool ok =
        (m_cfg.user[0] == '\0')
            ? m_impl->mqtt.connect(m_cfg.clientId, willTopic, 1, true,
                                   LWT_OFFLINE)
            : m_impl->mqtt.connect(m_cfg.clientId, m_cfg.user, m_cfg.pass,
                                   willTopic, 1, true, LWT_OFFLINE);
    if (!ok) {
        if (m_logger != nullptr) {
            m_logger->logWarn("mqtt", "connect failed");
        }
        return;
    }
    m_brokerLostSinceMs = 0;
    if (m_logger != nullptr) {
        m_logger->logInfo("mqtt", "connected");
    }
    // Retained "online": подписчики статуса сразу узнают о нашем присутствии.
    m_impl->mqtt.publish(willTopic, LWT_ONLINE, true);
    resubscribeAll();
}

void MqttManager::resubscribeAll() {
    for (uint8_t i = 0; i < m_subscriptionCount; ++i) {
        m_impl->mqtt.subscribe(m_subscriptions[i].topic);
    }
}

bool MqttManager::isConnected() const {
    return m_configured && m_impl != nullptr && m_impl->mqtt.connected();
}

bool MqttManager::publish(const char* topic, const char* payload,
                          bool retained) {
    if (!isConnected()) {
        return false;  // публикация в никуда молчаливо терялась бы
    }
    return m_impl->mqtt.publish(topic, payload, retained);
}

bool MqttManager::subscribe(const char* topic, IMessageHandler* handler) {
    Subscription* existing = findSubscription(topic);
    if (existing != nullptr) {
        existing->handler = handler;  // переподписка тем же топиком
        return true;
    }
    if (m_subscriptionCount >= MQTT_SUBSCRIPTIONS_MAX) {
        if (m_logger != nullptr) {
            m_logger->logWarn("mqtt", "subscriptions full");
        }
        return false;
    }
    Subscription& sub = m_subscriptions[m_subscriptionCount++];
    strncpy(sub.topic, topic, sizeof(sub.topic) - 1);
    sub.topic[sizeof(sub.topic) - 1] = '\0';  // strncpy не ставит '\0'
    sub.handler = handler;
    // При живом соединении подписываемся сразу; после reconnect —
    // resubscribeAll() восстановит подписку.
    if (m_impl != nullptr && m_impl->mqtt.connected()) {
        m_impl->mqtt.subscribe(sub.topic);
    }
    return true;
}

MqttManager::Subscription* MqttManager::findSubscription(const char* topic) {
    Subscription* wildcard = nullptr;
    for (uint8_t i = 0; i < m_subscriptionCount; ++i) {
        if (strcmp(m_subscriptions[i].topic, topic) == 0) {
            return &m_subscriptions[i];  // точное совпадение — приоритет
        }
        // Первый подходящий wildcard запоминаем: если точного нет, отдаём его.
        if (wildcard == nullptr && topicMatches(m_subscriptions[i].topic, topic)) {
            wildcard = &m_subscriptions[i];
        }
    }
    return wildcard;
}

bool MqttManager::topicMatches(const char* pattern, const char* topic) {
    // '+' совпадает ровно с одним сегментом топика (до следующего '/').
    // Точное сравнение сегментов — посимвольное, без аллокаций.
    while (*pattern != '\0' && *topic != '\0') {
        if (*pattern == '+') {
            while (*topic != '\0' && *topic != '/') {
                ++topic;  // пропускаем весь сегмент
            }
            ++pattern;
            continue;
        }
        if (*pattern != *topic) {
            return false;
        }
        ++pattern;
        ++topic;
    }
    return *pattern == *topic;  // оба шаблона закончились одновременно
}

void MqttManager::onMessageCallback(char* topic, uint8_t* payload,
                                    size_t length) {
    if (s_instance != nullptr) {
        s_instance->dispatchMessage(topic, payload, length);
    }
}

void MqttManager::dispatchMessage(const char* topic, const uint8_t* payload,
                                  size_t length) {
    Subscription* sub = findSubscription(topic);
    if (sub == nullptr) {
        return;  // чужая подписка (не должно случаться — см. subscribe())
    }
    // Payload из PubSubClient не null-terminated — копируем в локальный буфер
    // с ограничением MQTT_PAYLOAD_MAX (публикации больше лимита обрезаются).
    char buffer[MQTT_PAYLOAD_MAX];
    const size_t copyLen =
        length < sizeof(buffer) - 1 ? length : sizeof(buffer) - 1;
    memcpy(buffer, payload, copyLen);
    buffer[copyLen] = '\0';
    sub->handler->onMessage(topic, buffer);
}

void MqttManager::tryRebootIfBrokerLost(uint32_t now) {
    if (!m_cfg.enableRebootWatchdog || m_brokerLostSinceMs == 0) {
        return;
    }
    if (now - m_brokerLostSinceMs < m_cfg.rebootTimeoutMs) {
        return;
    }
    // Резерв: брокер недоступен слишком долго — полный рестарт, одних
    // retry уже недостаточно (поведение esp32-local-mqtt).
    if (m_logger != nullptr) {
        m_logger->logError("reboot", "mqtt-lost");
    }
    delay(100);  // дать уйти конверту в UART/очередь перед рестартом
    ESP.restart();
}

#endif  // ARDUINO