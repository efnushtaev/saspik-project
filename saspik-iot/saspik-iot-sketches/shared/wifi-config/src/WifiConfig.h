#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <WString.h>

// Максимальные размеры полей конфигурации
#define WIFI_CONFIG_SSID_LEN 33
#define WIFI_CONFIG_PASS_LEN 65
#define WIFI_CONFIG_HOST_LEN 65
#define WIFI_CONFIG_USER_LEN 33
#define WIFI_CONFIG_PASS_MQTT_LEN 65

#define WIFI_CONFIG_AP_TIMEOUT_MS 2000

// Сколько портал ждёт настройки, прежде чем перезагрузиться и повторить
// попытку STA. Без таймаута неудачный Wi-Fi на старте = вечный AP: MQTT не
// поднимается, данных и логов нет, и снаружи устройство выглядит мёртвым.
// Отсчёт сбрасывается любым HTTP-запросом и наличием подключённого клиента.
#define WIFI_CONFIG_PORTAL_TIMEOUT_MS 600000UL  // 10 минут

struct DeviceConfig {
    char wifiSsid[WIFI_CONFIG_SSID_LEN];
    char wifiPass[WIFI_CONFIG_PASS_LEN];
    char mqttHost[WIFI_CONFIG_HOST_LEN];
    uint16_t mqttPort;
    char mqttUser[WIFI_CONFIG_USER_LEN];
    char mqttPass[WIFI_CONFIG_PASS_MQTT_LEN];
};

class WifiConfigManager {
public:
    /**
     * @brief Начинает работу конфигуратора.
     *
     * Читает конфиг из NVS (namespace "saspik").
     * Если конфига нет или кнопка зажата более WIFI_CONFIG_AP_TIMEOUT_MS при старте —
     * поднимается AP с captive portal и возвращается false.
     * Иначе — подключение к WiFi (STA) и возвращается true.
     *
     * @param out       структура для заполнения конфигом
     * @param buttonPin GPIO внешней кнопки (INPUT_PULLUP, нажатие = LOW), или -1 если кнопки нет
     * @param defaults  значения по умолчанию (если NVS пуст), или nullptr
     * @return true, если устройство подключено к WiFi, false — если работает портал
     */
    bool begin(DeviceConfig& out, int8_t buttonPin = -1, const DeviceConfig* defaults = nullptr);

    /**
     * @brief Обслуживает captive portal (DNS + HTTP). Вызывать в loop().
     */
    void handlePortal();

    /**
     * @brief true, если сейчас активен режим портала.
     */
    bool isPortalMode() const;

    /**
     * @brief Очищает сохранённый конфиг из NVS.
     */
    void resetConfig();

    /**
     * @brief true, если конфиг уже сохранён в NVS (был хотя бы один POST /save).
     */
    bool hasConfig() const;

private:
    bool readConfig(DeviceConfig& out);
    void saveConfig(const DeviceConfig& cfg);
    bool hasSavedConfig() const;
    bool isButtonPressed(int8_t buttonPin);
    void startPortal();
    void stopPortal();
    void handleRoot();
    void handleScan();
    void handleSave();
    void handleReset();

    bool portalMode = false;
    String apSsid;
    // Время последнего HTTP-запроса к порталу (0 — страницу ещё не открывали).
    uint32_t lastPortalRequestMs = 0;
};

extern WifiConfigManager WifiConfig;
