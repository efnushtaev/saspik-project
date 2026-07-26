#ifndef ESP_NOW_TRANSPORT_H
#define ESP_NOW_TRANSPORT_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <CRC8.h>
#include "config.h"

/**
 * @brief Максимальное количество полей в одном ESP-NOW пакете
 */
constexpr size_t ESP_NOW_MAX_FIELDS = 16;

/**
 * @brief Максимальный размер ESP-NOW пакета (стандартный лимит ESP-NOW = 250 байт)
 */
constexpr size_t ESP_NOW_MAX_PACKET_SIZE = 250;

/**
 * @brief Структура поля для ESP-NOW протокола
 * 
 * Позволяет описывать произвольные поля данных без привязки
 * к конкретным датчикам.
 */
struct EspNowField {
    uint8_t id;         // Идентификатор поля (0-255)
    const void* data;   // Указатель на данные
    size_t size;        // Размер данных в байтах
};

/**
 * @brief Транспортный уровень для отправки данных по ESP-NOW (SRP, OCP)
 * 
 * Универсальный класс, не привязанный к конкретным полям данных.
 * Принимает массив EspNowField и сериализует их в бинарный протокол:
 * [timestamp(4)] [fieldId(1) + data(N)] ... [checksum(1)]
 */
class EspNowTransport {
public:
    /**
     * @param receiverMac MAC-адрес приёмника (6 байт)
     */
    EspNowTransport(const uint8_t* receiverMac);

    /**
     * @brief Инициализация WiFi и ESP-NOW
     */
    void begin();

    /**
     * @brief Отправка произвольного набора полей по ESP-NOW
     * 
     * @param fields Массив полей для отправки
     * @param fieldCount Количество полей в массиве
     * @param timestamp Временная метка (всегда добавляется первой)
     */
    void send(const EspNowField* fields, size_t fieldCount, unsigned long timestamp);

private:
    CRC8 _crc;
    uint8_t _receiverMac[6];

    int32_t _getWiFiChannel(const char* ssid);
    void _initWiFi();
};

#endif // ESP_NOW_TRANSPORT_H