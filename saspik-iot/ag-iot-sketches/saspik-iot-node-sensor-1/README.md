[↑ ag-iot-sketches](saspik-iot/ag-iot-sketches/README.md)

# saspik-iot-node-sensor-1

Прошивка для ESP32 с датчиками DHT-22 и поплавковым датчиком (геркон). Данные отправляются по ESP-NOW на приёмник.

## Аппаратная схема

| Пин | Назначение |
|-----|-----------|
| GPIO4  | DHT-22 (data) |
| GPIO13 | Поплавковый датчик (геркон, INPUT_PULLUP) |

## Цикл работы

1. Пробуждение из глубокого сна (каждые 2 секунды)
2. Чтение DHT-22 (температура, влажность)
3. Чтение поплавкового датчика (вода есть/нет)
4. Отправка данных по ESP-NOW
5. Вывод JSON в Serial
6. Глубокий сон на 2 секунды

## Структура проекта

```
src/
├── main.cpp                          # Оркестратор
├── config.h                          # Конфигурация (пины, MAC, таймауты)
├── sensors/
│   ├── ISensor.h                     # Интерфейс датчика
│   ├── Dht22Sensor.h/.cpp            # DHT-22
│   └── FloatSensor.h/.cpp            # Поплавковый датчик
├── communication/
│   └── EspNowTransport.h/.cpp        # ESP-NOW транспорт
├── system/
│   └── SleepManager.h/.cpp           # Управление глубоким сном
└── utils/
    ├── Filter.h/.cpp                 # Фильтрация сигналов
    └── Formatter.h/.cpp              # Форматирование JSON
```

## Бинарный протокол ESP-NOW

Данные передаются в сыром бинарном виде (не JSON, не текст).

### Формат пакета

```
[timestamp: 4 байта] [fieldId: 1 байт] [data: N байт] ... [checksum: 1 байт]
```

| Смещение | Размер | Поле | Описание |
|----------|--------|------|----------|
| 0 | 4 | `timestamp` | unsigned long, little-endian |
| 4 | 1 | `fieldId[0]` | Идентификатор первого поля (uint8_t) |
| 5 | N | `data[0]` | Данные первого поля (raw binary) |
| 5+N | 1 | `fieldId[1]` | Идентификатор второго поля |
| 6+N | M | `data[1]` | Данные второго поля |
| ... | ... | ... | ... |
| last | 1 | `checksum` | CRC8 от всех байт ДО checksum |

### Идентификаторы полей

| ID | Поле | Тип | Размер |
|----|------|-----|--------|
| 1 | temperature | float | 4 байта |
| 2 | humidity | float | 4 байта |
| 3 | float_sensor | float | 4 байта |

### Пример пакета (20 байт)

```
Байт  0-3:   timestamp    (unsigned long)
Байт  4:     0x01         (FIELD_TEMPERATURE)
Байт  5-8:   temperature  (float)
Байт  9:     0x02         (FIELD_HUMIDITY)
Байт 10-13:  humidity     (float)
Байт 14:     0x03         (FIELD_FLOAT_SENSOR)
Байт 15-18:  floatState   (float)
Байт 19:     CRC8         (от байт 0-18)
```

### Разбор на приёмнике

1. Прочитать первые 4 байта как `unsigned long` (timestamp)
2. В цикле читать по 1 байту (fieldId), затем известное количество байт данных для этого fieldId
3. Последний байт — CRC8. Вычислить CRC8 от всех байт до него и сверить.

## Конфигурация

Все настройки в [`src/config.h`](saspik-iot/ag-iot-sketches/saspik-iot-node-sensor-1/src/config.h):

- `WIFI_SSID` / `WIFI_PASS` — WiFi сеть для синхронизации канала
- `ESP_NOW_RECEIVER_MAC` — MAC-адрес приёмника ESP-NOW
- `SLEEP_SECONDS` — интервал глубокого сна (сек)
- `SAMPLES` / `SAMPLE_DELAY` — количество и задержка сэмплов для фильтрации
- `STAB_DELAY` — задержка стабилизации датчиков (мс)

## Зависимости (platformio.ini)

- `adafruit/DHT sensor library` — DHT-22
- `robtillaart/CRC` — CRC8 для ESP-NOW

## Структура каталога

```
.
├── include/
├── lib/
├── platformio.ini
├── README.md
├── src/
├── test/
└── .vscode/
```