// SPDX-License-Identifier: MIT
/*
 * config.h — единственное место compile-time констант boilerplate.
 *
 * Правило: всё, что известно до компиляции (роль, unitId/objectId, пины,
 * интервалы, таймауты, словарь полей ESP-NOW, таблица узлов) — здесь.
 * Всё, что меняется в рантайме (WiFi/MQTT через captive portal, параметры
 * приложения через NVS) — вне этого файла (см. ConfigManager).
 *
 * Файл намеренно не зависит от Arduino: его включают и прошивка, и
 * host-тесты (env:native), поэтому здесь только стандартные типы.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

// ======================== Роль устройства ========================
// Задаётся build-флагом -DDEVICE_ROLE=N (см. platformio.ini).

#define DEVICE_ROLE_GATEWAY 1  ///< Шлюз: WiFi + MQTT + ESP-NOW мост
#define DEVICE_ROLE_SENSOR 2   ///< Сенсор: ESP-NOW + light sleep + шины
#define DEVICE_ROLE_DEVICE 3   ///< Устройство: ESP-NOW rx/tx + шины

#ifndef DEVICE_ROLE
#error "DEVICE_ROLE must be defined via build_flags (-DDEVICE_ROLE=1|2|3)"
#endif

// ======================== Идентификация объекта ========================
// Формат шифра: saspik-{роли}{протокол}{питание}{nnn} (docs/classification.md).
// Роль шаблона задаёт свой шифр по умолчанию; серийный номер меняется под устройство.

constexpr size_t DEVICE_INFO_UNIT_LEN = 16;    ///< Максимум unitId с учётом '\0'
constexpr size_t DEVICE_INFO_OBJECT_LEN = 24;  ///< Максимум objectId с учётом '\0'

#if DEVICE_ROLE == DEVICE_ROLE_GATEWAY
constexpr char UNIT_ID[] = "unitId2";
constexpr char OBJECT_ID[] = "saspik-gwqm001";  // Gateway, WiFi+MQTT(wq), mains(m)
constexpr char CLIENT_ID_PREFIX[] = "saspik-gw";
#elif DEVICE_ROLE == DEVICE_ROLE_SENSOR
constexpr char UNIT_ID[] = "unitId2";
constexpr char OBJECT_ID[] = "saspik-senb001";  // Sensor, ESP-NOW(en), battery(b)
constexpr char CLIENT_ID_PREFIX[] = "saspik-s";
#else  // DEVICE_ROLE_DEVICE
constexpr char UNIT_ID[] = "unitId2";
constexpr char OBJECT_ID[] = "saspik-aenm001";  // Device(actuator), ESP-NOW(en), mains(m)
constexpr char CLIENT_ID_PREFIX[] = "saspik-d";
#endif

// ======================== Пины (система и периферия вне шин) ========================
// Пины ДВУХ видов, поэтому два раздела:
// 1. Здесь — система и периферия, которая НЕ ходит через шины InterfaceManager:
//    кнопка портала, светодиод, примеры датчиков/исполнителей на прямом GPIO
//    (DHT22 — собственный протокол; геркон/реле — цифровые входы/выходы).
// 2. Раздел «Шины» ниже — пины интерфейсов I2C/OneWire/SPI/UART, которыми
//    владеет InterfaceManager.
// Все пины заданы в config.h — в коде нет ни одного жёстко зашитого GPIO.

// Префикс PIN_GPIO_N — пин используется как прямой цифровой вход/выход (сырой GPIO);
// назначение каждого пина описано в комментарии рядом (LED, кнопка портала, DHT22,
// геркон, реле). Пины шин (I2C/OneWire/SPI/UART) именуются PIN_* с именем сигнала —
// это пин конкретного сигнала интерфейса, его владелец InterfaceManager.
constexpr uint8_t PIN_GPIO_1 = 2;            ///< Встроенный светодиод (пример управления)
constexpr uint8_t PIN_GPIO_2 = 25; ///< Кнопка captive portal (INPUT_PULLUP, LOW = нажата)
constexpr uint8_t PIN_GPIO_3 = 4;            ///< Пример датчика: DHT22 (ISensor, свой протокол)
constexpr uint8_t PIN_GPIO_4 = 13;         ///< Пример датчика: поплавковый геркон (ISensor, GPIO-вход)
constexpr uint8_t PIN_GPIO_5 = 26;         ///< Пример исполнителя: реле (IActuator, GPIO-выход)

// ======================== Шины (InterfaceManager) ========================
// Включайте только используемые шины: неиспользуемая не создаётся и не инициализируется.

constexpr bool BUS_I2C_ENABLED = false;
constexpr uint8_t PIN_I2C_SDA = 21;
constexpr uint8_t PIN_I2C_SCL = 22;
constexpr uint32_t I2C_FREQ_HZ = 100000UL;

// ВАЖНО: пин шины OneWire (27) не должен совпадать с PIN_GPIO_3 (4, пример DHT22) —
// DHT22 использует собственный протокол на своём пине, а шина OneWire — свой.
constexpr bool BUS_ONE_WIRE_ENABLED = false;
constexpr uint8_t PIN_ONE_WIRE = 27;

constexpr bool BUS_SPI_ENABLED = false;
constexpr uint8_t PIN_SPI_SCK = 18;
constexpr uint8_t PIN_SPI_MISO = 19;
constexpr uint8_t PIN_SPI_MOSI = 23;
constexpr uint8_t PIN_SPI_CS = 5;

constexpr bool BUS_UART_ENABLED = false;
constexpr uint8_t UART_NUM = 1;  // 1 = Serial1, 2 = Serial2
constexpr uint8_t PIN_UART_TX = 17;
constexpr uint8_t PIN_UART_RX = 16;
constexpr uint32_t UART_BAUD = 9600UL;

// ======================== MQTT (дефолты для формы портала) ========================
// Реальные значения хранятся в NVS и редактируются через captive portal
// (shared/wifi-config, переиспользуется как есть). Эти константы — только
// подстановка по умолчанию в форму при первом запуске.

constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASS[] = "";
constexpr char MQTT_BROKER[] = "192.168.0.118";
constexpr uint16_t MQTT_PORT = 1883;
constexpr char MQTT_USER[] = "admin";
constexpr char MQTT_PASS[] = "password123";

// Таймауты и интервалы (значения перенесены из esp32-local-mqtt, где они
// проверены в бою; комментарии о причинах сохранены).

// Минимальный интервал между попытками подключения к MQTT. Предотвращает
// блокировку loop() на connect при недоступном брокере (иначе Task WDT reset).
constexpr uint32_t MQTT_RECONNECT_INTERVAL_MS = 3000UL;
// Таймаут TCP-соединения к брокеру — чтобы connect не висел дольше WDT.
constexpr uint32_t MQTT_CONNECT_TIMEOUT_MS = 4000UL;
// Непрерывная недоступность MQTT дольше этого → авто-ребут (не для Sensor).
constexpr uint32_t MQTT_REBOOT_TIMEOUT_MS = 900000UL;  // 15 минут
// Непрерывная недоступность WiFi дольше этого → авто-ребут (не для Sensor).
constexpr uint32_t WIFI_REBOOT_TIMEOUT_MS = 300000UL;  // 5 минут
// Минимальный интервал между попытками восстановления WiFi.
constexpr uint32_t WIFI_RECONNECT_INTERVAL_MS = 10000UL;
// Период публикации диагностики во время длительного сбоя.
constexpr uint32_t DIAG_PUBLISH_INTERVAL_MS = 60000UL;

// Лимиты протокола (защита буферов, не магические числа).
constexpr uint16_t MQTT_PAYLOAD_MAX = 128;       ///< Максимальный размер входящего payload
constexpr uint8_t MQTT_SUBSCRIPTIONS_MAX = 8;    ///< Максимум одновременных подписок
constexpr size_t MQTT_TOPIC_MAX_LEN = 64;        ///< Максимум длины топика с учётом '\0'

// ======================== ESP-NOW ========================
// Канал должен совпадать с каналом WiFi сети шлюза (см. plans/boilerplate-architecture.md,
// открытый вопрос о синхронизации канала).

constexpr uint8_t ESP_NOW_CHANNEL = 1;
constexpr uint8_t ESP_NOW_MAX_FIELDS = 16;   ///< Поля в одном пакете (лимит узла node-sensor-1)
constexpr uint8_t ESP_NOW_MAX_PEERS = 4;     ///< Максимум пиров (v1: шлюз + резерв)
constexpr size_t ESP_NOW_PACKET_MAX = 250;   ///< Стандартный лимит payload ESP-NOW

// Типы сообщений ESP-NOW (первый байт пакета, см. EspNowCodec).
constexpr uint8_t ESP_NOW_TYPE_DATA = 1;     ///< Данные с датчиков (сенсор → шлюз)
constexpr uint8_t ESP_NOW_TYPE_LOG = 2;      ///< JSON-конверт лога (узел → шлюз)
constexpr uint8_t ESP_NOW_TYPE_COMMAND = 3;  ///< Команда (шлюз → устройство)
constexpr uint8_t ESP_NOW_TYPE_ACK = 4;      ///< Подтверждение приёма команды

// Служебные field id внутри пакета (логи/команды несут готовый JSON).
constexpr uint8_t ESP_NOW_FIELD_LOG_JSON = 0xFF;
constexpr uint8_t ESP_NOW_FIELD_COMMAND_JSON = 0xFE;

// Идентификаторы полей примерных датчиков (должны совпадать со словарём ниже).
constexpr uint8_t FIELD_ID_TEMPERATURE = 1;
constexpr uint8_t FIELD_ID_HUMIDITY = 2;
constexpr uint8_t FIELD_ID_FLOAT = 3;

// Шлюз по умолчанию для сенсора/устройства (v1 — один фиксированный peer;
// runtime-управление списком пиров — TODO, см. TODO.md).
constexpr uint8_t DEFAULT_GATEWAY_MAC[6] = {0xD4, 0xE9, 0xF4, 0xF3, 0x97, 0x38};

// ======================== Словарь полей и таблица узлов (Gateway) ========================
// Мост переводит fieldId ↔ имя поля JSON. Таблица узлов: objectId узла ↔ его MAC.

struct FieldEntry {
    uint8_t id;         ///< fieldId из ESP-NOW-пакета
    const char* key;    ///< имя ключа в JSON (mqtt-topics.md: sensor/{unitId}/{objectId})
};

struct NodeEntry {
    const char* objectId;  ///< objectId узла (уникален в рамках юнита)
    uint8_t mac[6];        ///< MAC узла в ESP-NOW
};

// Пока все поля данных — float; поддержка int/bool/string — TODO (см. TODO.md).
constexpr FieldEntry FIELD_DICTIONARY[] = {
    {FIELD_ID_TEMPERATURE, "temperature"},
    {FIELD_ID_HUMIDITY, "humidity"},
    {FIELD_ID_FLOAT, "float"},
    {ESP_NOW_FIELD_LOG_JSON, "log"},  // служебное: конверт лога передаётся as-is
};
constexpr size_t FIELD_DICTIONARY_SIZE =
    sizeof(FIELD_DICTIONARY) / sizeof(FIELD_DICTIONARY[0]);

// Таблица узлов шлюза. Заполняется MAC реальных узлов; пустая строка objectId
// означает «нет узлов» (тогда мост работает как чистый пересыльщик без маршрутизации).
constexpr NodeEntry NODE_TABLE[] = {
    {"saspik-senb001", {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
};
constexpr size_t NODE_TABLE_SIZE = sizeof(NODE_TABLE) / sizeof(NODE_TABLE[0]);

// ======================== Интервалы приложения ========================
// Значения по умолчанию; переопределяются в рантайме через NVS (ConfigManager).

constexpr uint32_t SENSOR_INTERVAL_MS = 2000UL;   ///< Чтение/отправка данных (все шаблоны)
constexpr uint32_t SLEEP_INTERVAL_MS = 60000UL;   ///< Light sleep сенсора (Sensor)
constexpr uint32_t WAKE_CHECK_INTERVAL_MS = 1000UL; ///< Проверка «пора спать» в цикле
constexpr size_t TIMER_TASKS_MAX = 8;             ///< Вместимость TimerRegistry

// ======================== Логирование ========================
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint16_t LOG_MSG_MAX_LEN = 80;          ///< Ограничение текста события (буферы фиксированы)
constexpr uint16_t LOG_ENVELOPE_MAX_LEN = 256;    ///< Размер JSON-конверта
constexpr uint8_t LOG_QUEUE_SIZE = 8;             ///< Очередь MqttLogSink при offline
constexpr uint8_t LOG_SINKS_MAX = 4;              ///< Максимум sink'ов у Logger

// ======================== Примеры периферии ========================
// Примерные датчики/исполнители лежат в src/app/examples и показывают,
// как пользоваться интерфейсами ISensor/IActuator. Мок-режим повторяет
// поведение esp32-local-mqtt (плавные псевдо-данные без железа).

constexpr bool USE_MOCK_SENSOR = true;            ///< DHT22: мок вместо реального чтения
constexpr bool USE_EXAMPLE_DHT22 = true;          ///< Подключить пример DHT22 (Sensor/Device/Gateway-свои)
constexpr bool USE_EXAMPLE_FLOAT = false;         ///< Подключить пример поплавкового датчика
constexpr bool USE_EXAMPLE_RELAY = false;         ///< Подключить пример реле (Device/Gateway)
constexpr uint8_t DHT_TYPE = 22;                  ///< DHT22 = 22