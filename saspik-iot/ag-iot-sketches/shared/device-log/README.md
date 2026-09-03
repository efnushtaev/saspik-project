# device-log

Энергонезависимое кольцевое логирование и причина ребута для ESP32 (Arduino / PlatformIO). Позволяет хранить хвост лога событий на **LittleFS** и фиксировать причину последнего перезапуска в **NVS** — данные переживают перезагрузку и потерю питания.

## Возможности

- **LittleFS-лог**: кольцевой буфер в файле `/log.txt` (лимит 16 КБ), строки вида `[uptime] событие`
- **NVS-причина ребута**: `setRebootCause()`/`rebootCause()` — причина последнего ребута, не сбрасывается после чтения
- Ротация: при выходе за лимит старые строки отсекаются
- API для чтения хвоста лога: `readTail()` — подходит для публикации в MQTT при старте
- Без внешних зависимостей: `LittleFS` и `Preferences` из ESP32 Arduino Core

## Подключение к проекту

В `platformio.ini` добавьте путь к общему каталогу библиотек и файловую систему `littlefs`:

```ini
lib_extra_dirs =
    ../shared

board_build.filesystem = littlefs
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
    DeviceLog.write("=== started, reason=%s ===",
                    DeviceLog.rebootCause()[0] ? DeviceLog.rebootCause() : "none");
}

void loop() {
    // всё отлично
    DeviceLog.write("sensor read ok");

    // произошёл сбой — фиксируем причину и перезагружаемся
    DeviceLog.setRebootCause("mqtt-timeout");
    DeviceLog.write("=== rebooting, reason=mqtt-timeout ===");
    ESP.restart();
}
```

### Хвост лога для MQTT

```cpp
char tail[512];
size_t n = DeviceLog.readTail(tail, sizeof(tail));
if (n > 0) {
    mqttClient.publish(TOPIC_DIAG, tail);
}
```

## API

| Метод | Описание |
|---|---|
| `bool begin()` | Инициализирует LittleFS и NVS |
| `bool write(fmt, ...)` | Пишет строку в лог (printf-формат), добавляет uptime-штамп |
| `size_t readTail(char* buf, size_t len)` | Читает хвост лога (последние строки) |
| `void clear()` | Очищает лог |
| `void setRebootCause(const char* cause)` | Сохраняет причину ребута в NVS |
| `const char* rebootCause()` | Возвращает причину последнего ребута (или пустую строку) |
| `bool hasRebootCause()` | Есть ли сохранённая причина |

Глобальный экземпляр: `DeviceLog`.

## Причины ребута в esp32-local-mqtt

Используются строки: `wifi-lost` (не поднялся Wi-Fi) и `mqtt-timeout` (долгая недоступность брокера).

## Структура

```
device-log/
├── library.json
└── src/
    ├── DeviceLog.h      # API + глобальный экземпляр
    └── DeviceLog.cpp    # LittleFS-кольцевой лог + NVS-причина ребута
```
