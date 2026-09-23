#include "WifiConfig.h"
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <Preferences.h>
#include "web/index_html.h"

// ---------------------------------------------------------------------------
// Статические компоненты (создаются только в режиме портала)
// ---------------------------------------------------------------------------
static DNSServer dnsServer;
static WebServer* httpServer = nullptr;

static const char NVS_NAMESPACE[] = "saspik";
static const char KEY_SSID[]      = "wifi_ssid";
static const char KEY_PASS[]      = "wifi_pass";
static const char KEY_MQTT_HOST[] = "mqtt_host";
static const char KEY_MQTT_PORT[] = "mqtt_port";
static const char KEY_MQTT_USER[] = "mqtt_user";
static const char KEY_MQTT_PASS[] = "mqtt_pass";

WifiConfigManager WifiConfig;

// ---------------------------------------------------------------------------
// Внутренние утилиты
// ---------------------------------------------------------------------------

static String apNameFromMac() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char name[16];
    snprintf(name, sizeof(name), "SASPIK-%02X%02X%02X", mac[3], mac[4], mac[5]);
    return String(name);
}

// ---------------------------------------------------------------------------
// NVS
// ---------------------------------------------------------------------------

bool WifiConfigManager::hasSavedConfig() const {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) return false;
    bool ok = prefs.isKey(KEY_SSID) && prefs.isKey(KEY_MQTT_HOST);
    prefs.end();
    return ok;
}

bool WifiConfigManager::hasConfig() const {
    return hasSavedConfig();
}

bool WifiConfigManager::readConfig(DeviceConfig& out) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) return false;
    if (!prefs.isKey(KEY_SSID) || !prefs.isKey(KEY_MQTT_HOST)) {
        prefs.end();
        return false;
    }
    String ssid = prefs.getString(KEY_SSID, "");
    String pass = prefs.getString(KEY_PASS, "");
    String host = prefs.getString(KEY_MQTT_HOST, "");
    uint16_t port = prefs.getUShort(KEY_MQTT_PORT, 1883);
    String user = prefs.getString(KEY_MQTT_USER, "");
    String passMqtt = prefs.getString(KEY_MQTT_PASS, "");
    prefs.end();

    ssid.toCharArray(out.wifiSsid, WIFI_CONFIG_SSID_LEN);
    pass.toCharArray(out.wifiPass, WIFI_CONFIG_PASS_LEN);
    host.toCharArray(out.mqttHost, WIFI_CONFIG_HOST_LEN);
    out.mqttPort = port;
    user.toCharArray(out.mqttUser, WIFI_CONFIG_USER_LEN);
    passMqtt.toCharArray(out.mqttPass, WIFI_CONFIG_PASS_MQTT_LEN);

    return out.wifiSsid[0] != '\0' && out.mqttHost[0] != '\0';
}

void WifiConfigManager::saveConfig(const DeviceConfig& cfg) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) return;
    prefs.putString(KEY_SSID, cfg.wifiSsid);
    prefs.putString(KEY_PASS, cfg.wifiPass);
    prefs.putString(KEY_MQTT_HOST, cfg.mqttHost);
    prefs.putUShort(KEY_MQTT_PORT, cfg.mqttPort);
    prefs.putString(KEY_MQTT_USER, cfg.mqttUser);
    prefs.putString(KEY_MQTT_PASS, cfg.mqttPass);
    prefs.end();
    Serial.println("[wifi-config] config saved to NVS");
}

void WifiConfigManager::resetConfig() {
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.remove(KEY_SSID);
        prefs.remove(KEY_PASS);
        prefs.remove(KEY_MQTT_HOST);
        prefs.remove(KEY_MQTT_PORT);
        prefs.remove(KEY_MQTT_USER);
        prefs.remove(KEY_MQTT_PASS);
        prefs.end();
        Serial.println("[wifi-config] NVS config cleared");
    }
}

// ---------------------------------------------------------------------------
// Кнопка
// ---------------------------------------------------------------------------

bool WifiConfigManager::isButtonPressed(int8_t buttonPin) {
    if (buttonPin < 0) return false;
    pinMode(buttonPin, INPUT_PULLUP);
    uint32_t start = millis();
    while (millis() - start < WIFI_CONFIG_AP_TIMEOUT_MS) {
        if (digitalRead(buttonPin) == LOW) return true;
        delay(20);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Публичный API
// ---------------------------------------------------------------------------

bool WifiConfigManager::begin(DeviceConfig& out, int8_t buttonPin, const DeviceConfig* defaults) {
    if (defaults) {
        out = *defaults;
    } else {
        memset(&out, 0, sizeof(out));
        out.mqttPort = 1883;
    }

    bool saved = readConfig(out);
    bool button = isButtonPressed(buttonPin);

    if (saved && !button) {
        // Штатный режим: подключаемся к WiFi
        WiFi.mode(WIFI_STA);
        WiFi.begin(out.wifiSsid, out.wifiPass);
        Serial.printf("[wifi-config] connecting to %s", out.wifiSsid);

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[wifi-config] connected, IP: %s\n", WiFi.localIP().toString().c_str());
            portalMode = false;
            return true;
        }
        Serial.println("[wifi-config] WiFi connect failed, starting portal");
    }

    if (!saved && button) {
        Serial.println("[wifi-config] no config + button pressed, starting portal");
    } else if (!saved) {
        Serial.println("[wifi-config] no config in NVS, starting portal");
    } else {
        Serial.println("[wifi-config] button pressed, starting portal");
    }

    startPortal();
    portalMode = true;
    return false;
}

void WifiConfigManager::startPortal() {
    apSsid = apNameFromMac();

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str());

    dnsServer.start(53, "*", WiFi.softAPIP());

    httpServer = new WebServer(80);
    httpServer->on("/", HTTP_GET, [this]() { handleRoot(); });
    httpServer->on("/scan", HTTP_GET, [this]() { handleScan(); });
    httpServer->on("/save", HTTP_POST, [this]() { handleSave(); });
    httpServer->on("/reset", HTTP_POST, [this]() { handleReset(); });
    httpServer->begin();

    Serial.printf("[wifi-config] AP started: %s (%s)\n", apSsid.c_str(), WiFi.softAPIP().toString().c_str());
    Serial.println("[wifi-config] captive portal on http://192.168.4.1/");
}

void WifiConfigManager::stopPortal() {
    if (httpServer) {
        httpServer->stop();
        delete httpServer;
        httpServer = nullptr;
    }
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
}

bool WifiConfigManager::isPortalMode() const {
    return portalMode;
}

void WifiConfigManager::handlePortal() {
    if (!portalMode) return;
    dnsServer.processNextRequest();
    if (httpServer) httpServer->handleClient();
}

// ---------------------------------------------------------------------------
// HTTP-обработчики
// ---------------------------------------------------------------------------

void WifiConfigManager::handleRoot() {
    httpServer->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    httpServer->send(200, "text/html", INDEX_HTML);
}

void WifiConfigManager::handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i) json += ",";
        json += "{";
        json += "\"ssid\":\"";
        json += WiFi.SSID(i);
        json += "\",";
        json += "\"rssi\":";
        json += WiFi.RSSI(i);
        json += "}";
    }
    json += "]";
    WiFi.scanDelete();
    httpServer->send(200, "application/json", json);
}

void WifiConfigManager::handleSave() {
    DeviceConfig cfg;
    memset(&cfg, 0, sizeof(cfg));

    String ssid = httpServer->arg("ssid");
    ssid.trim();
    if (ssid.length() == 0) {
        httpServer->send(400, "text/plain", "SSID is required");
        return;
    }

    ssid.toCharArray(cfg.wifiSsid, WIFI_CONFIG_SSID_LEN);

    String host = httpServer->arg("mqtt_host");
    host.trim();
    if (host.length() == 0) {
        httpServer->send(400, "text/plain", "MQTT host is required");
        return;
    }

    httpServer->arg("pass").toCharArray(cfg.wifiPass, WIFI_CONFIG_PASS_LEN);
    host.toCharArray(cfg.mqttHost, WIFI_CONFIG_HOST_LEN);
    String port = httpServer->arg("mqtt_port");
    cfg.mqttPort = (uint16_t)(port.toInt() > 0 ? port.toInt() : 1883);
    httpServer->arg("mqtt_user").toCharArray(cfg.mqttUser, WIFI_CONFIG_USER_LEN);
    httpServer->arg("mqtt_pass").toCharArray(cfg.mqttPass, WIFI_CONFIG_PASS_MQTT_LEN);

    saveConfig(cfg);

    String html = FPSTR(SUCCESS_HTML);
    html.replace("%SSID%", ssid);
    httpServer->send(200, "text/html", html);

    delay(3000);
    stopPortal();
    portalMode = false;
    ESP.restart();
}

void WifiConfigManager::handleReset() {
    resetConfig();
    httpServer->send(200, "text/html",
        "<html><body><h3>Config cleared. Rebooting...</h3></body></html>");
    delay(500);
    ESP.restart();
}
