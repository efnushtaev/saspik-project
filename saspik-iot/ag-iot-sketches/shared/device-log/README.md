# device-log

Энергонезависимое хранение причины ребута для ESP32 (Arduino / PlatformIO) в **NVS**. Диагностические события публикуются в MQTT в едином JSON-конверте; здесь фиксируется только причина последнего перезапуска, которая переживает перезагрузку и потерю питания.

> История: ранее библиотека вела кольцевой файловый лог на **LittleFS**. Запись в LittleFS во время активного WiFi вызывала зависания/ресеты (flash-latency и нестабильность питания). По решению от файлового лога **отказались** — осталась только запись reboot-cause в NVS + публикация событий в MQTT.

## Возможности

- **NVS-причина ребута**: `setRebootCause()`/`rebootCause()` — причина последнего ребута, не сбрасывается после чтения
- Редкая, малая запись в NVS — минимум flash-операций, стабильность при активном WiFi
- Без внешних зависимостей: `Preferences` из ESP32 Arduino Core

## Подключение к проекту

В `platformio.ini` добавьте путь к общему каталогу библиотек:

```ini
lib_extra_dirs =
    ../shared
```

## Использование

```cpp
#include "DeviceLog.h"

void setup() {
    DeviceLog.begin();

    // Причина предыдущего ребута (из NVS)
    if (DeviceLog.hasRebootCause()) {
        Serial.println(DeviceLog.rebootCause());
    }
}

void loop() {
    // произошёл сбой — фиксируем причину и перезагружаемся
    DeviceLog.setRebootCause("mqtt-timeout");
    ESP.restart();
}
```

## API

| Метод | Описание |
|---|---|
| `bool begin()` | Инициализирует менеджер и загружает причину ребута из NVS |
| `void setRebootCause(const char* cause)` | Сохраняет причину ребута в NVS |
| `const char* rebootCause()` | Возвращает причину последнего ребута (или пустую строку) |
| `bool hasRebootCause()` | Есть ли сохранённая причина |

Глобальный экземпляр: `DeviceLog`.

## Причины ребута в esp32-local-mqtt

Используются строки: `wifi-lost` (не поднялся Wi-Fi) и `mqtt-timeout` (долгая недоступность брокера). Причина отправляется на старте в MQTT-конверте (`event="startup"`, поле `cause`).

## Структура

```
device-log/
├── library.json
└── src/
    ├── DeviceLog.h      # API + глобальный экземпляр
    └── DeviceLog.cpp    # NVS-причина ребута
```
