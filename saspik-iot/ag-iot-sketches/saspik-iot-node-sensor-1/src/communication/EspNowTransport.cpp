#include "EspNowTransport.h"

EspNowTransport::EspNowTransport(const uint8_t* receiverMac)
{
    memcpy(_receiverMac, receiverMac, 6);
}

// ----------------------------------------------------------------------------
// WiFi
// ----------------------------------------------------------------------------

int32_t EspNowTransport::_getWiFiChannel(const char* ssid)
{
    if (int32_t n = WiFi.scanNetworks())
    {
        for (uint8_t i = 0; i < n; i++)
        {
            if (!strcmp(ssid, WiFi.SSID(i).c_str()))
            {
                return WiFi.channel(i);
            }
        }
    }
    return 0;
}

void EspNowTransport::_initWiFi()
{
    WiFi.mode(WIFI_MODE_STA);

    int32_t channel = _getWiFiChannel(WIFI_SSID);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

// ----------------------------------------------------------------------------
// ESP-NOW
// ----------------------------------------------------------------------------

void EspNowTransport::begin()
{
    _initWiFi();

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW: failed to initialize");
        while (1)
            ;
    }

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, _receiverMac, 6);
    peerInfo.ifidx = (wifi_interface_t)ESP_IF_WIFI_STA;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("ESP-NOW: pairing failure");
        while (1)
            ;
    }
}

void EspNowTransport::send(const EspNowField* fields, size_t fieldCount, unsigned long timestamp)
{
    if (fields == nullptr || fieldCount == 0 || fieldCount > ESP_NOW_MAX_FIELDS)
    {
        Serial.println("ESP-NOW: invalid fields");
        return;
    }

    uint8_t buffer[ESP_NOW_MAX_PACKET_SIZE];
    size_t offset = 0;

    // 1. Временная метка (всегда идёт первой)
    memcpy(buffer + offset, &timestamp, sizeof(timestamp));
    offset += sizeof(timestamp);

    // 2. Поля данных
    for (size_t i = 0; i < fieldCount; i++)
    {
        if (fields[i].data == nullptr || fields[i].size == 0)
        {
            continue; // Пропускаем пустые поля
        }

        // Проверка, что поле влезет в буфер
        if (offset + 1 + fields[i].size + 1 > ESP_NOW_MAX_PACKET_SIZE)
        {
            Serial.println("ESP-NOW: buffer overflow, truncating");
            break;
        }

        // ID поля (1 байт)
        buffer[offset] = fields[i].id;
        offset += 1;

        // Данные поля
        memcpy(buffer + offset, fields[i].data, fields[i].size);
        offset += fields[i].size;
    }

    // 3. Контрольная сумма
    _crc.reset();
    _crc.add(buffer, offset);
    uint8_t checksum = _crc.calc();
    memcpy(buffer + offset, &checksum, sizeof(checksum));
    offset += sizeof(checksum);

    // 4. Отправка
    esp_err_t result = esp_now_send(_receiverMac, buffer, offset);
    if (result == ESP_OK)
    {
        Serial.print("ESP-NOW: отправлено ");
        Serial.print(offset);
        Serial.println(" байт");
    }
    else
    {
        Serial.println("ESP-NOW: ошибка отправки");
    }
}