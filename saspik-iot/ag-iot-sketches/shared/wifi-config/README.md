# wifi-config

Универсальный captive portal для ESP32 (Arduino / PlatformIO): конфигурация **Wi-Fi и MQTT** без перепрошивки. Настройки сохраняются в **NVS** (`Preferences`) и переживают перезагрузку и потерю питания.

## Возможности

- Captive portal (DNS-перехват → `192.168.4.1`) на AP `SASPIK-XXXX`
- Форма: SSID/пароль Wi-Fi, MQTT хост/порт/логин/пароль
- Кнопка «Сканировать Wi-Fi» (`GET /scan` → JSON список сетей)
- Кнопки «Сохранить и перезагрузить» (`POST /save`) и «Сбросить настройки» (`POST /reset`)
- Активация: **первое включение** (нет конфига в NVS) или **кнопка при старте**
- Без внешних зависимостей: `Preferences`, `DNSServer`, `WebServer` из ESP32 Arduino Core

## Подключение к проекту

В `platformio.ini` добавьте путь к общему каталогу библиотек:

```ini
lib_extra_dirs =
    ../shared
```

## Использование

```cpp
#include "WifiConfig.h"

DeviceConfig config;
void setup() {
    if (!WifiConfig.begin(config, CONFIG_BUTTON_PIN)) {
        return;  // работает портал, loop() крутит handlePortal()
    }
    // config.wifiSsid, config.mqttHost, config.mqttPort, config.mqttUser, config.mqttPass
}

void loop() {
    WifiConfig.handlePortal();
    // ... основная логика (MQTT и т.п.)
}
```

### Параметры `begin()`

```cpp
bool begin(DeviceConfig& out, int8_t buttonPin = -1, const DeviceConfig* defaults = nullptr);
```

- `out` — структура, заполняется рабочим конфигом
- `buttonPin` — GPIO внешней кнопки (INPUT_PULLUP, нажатие = LOW), `-1` = нет кнопки
- `defaults` — дефолтные значения (например из `env_config.h`/`config.h`), если NVS пуст
- возвращает `true` — подключено к Wi-Fi (штатный режим), `false` — активен портал

### Пин кнопки

Пин задаётся в `config.h` каждого скетча:

```cpp
#define CONFIG_BUTTON_PIN 15
```

## Структура

```
wifi-config/
├── library.json
├── src/
│   ├── WifiConfig.h      # API + DeviceConfig
│   ├── WifiConfig.cpp    # NVS, DNS, WebServer, AP
│   └── web/
│       └── index_html.h  # HTML-страница портала
```
