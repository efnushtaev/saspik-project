# ESP32 Controller (ESP-NOW → MQTT Gateway)

[↑ ag-iot-sketches](saspik-iot/ag-iot-sketches/README.md)

ESP32-шлюз, который собирает данные от узлов по ESP-NOW, парсит бинарный протокол с CRC8 и публикует JSON в MQTT кластера SASPIK. Также подписывается на relay-топики для управления нагрузками.

## Как это работает

- **ESP-NOW** — приём данных от node-устройств (температура, влажность, поплавковый датчик)
- **NTP** — синхронизация времени для ISO 8601 меток в публикациях
- **MQTT** — публикация JSON в топики `sensor/{unitId}/dht22` и `sensor/{unitId}/float-1` каждые 5с
- **Relay control** — подписка на `units/{unitId}/commands/a_relay1-4`, управление GPIO через callback
- **Два ядра FreeRTOS** — ESP-NOW на core 0, MQTT на core 1

## Форматы топиков

Подробное описание всех топиков и форматов сообщений — в [`saspik-cluster/docs/mqtt-topics.md`](../../saspik-cluster/docs/mqtt-topics.md).

## Конфигурация

Все настройки в [`src/config.h`](saspik-iot/ag-iot-sketches/esp32-controller/src/config.h):
- WiFi SSID / пароль
- MQTT сервер, порт, логин/пароль
- Unit ID, топики сенсоров и реле
- Пины реле
- NTP сервер, интервал публикации

## Зависимости (platformio.ini)

- `PubSubClient` — MQTT
- `ArduinoJson` — формирование JSON
- `CRC8` — проверка целостности ESP-NOW пакетов

## Структура каталога

```
.
├── include/
├── lib/
├── platformio.ini
├── README.md
├── src/
│   ├── config.h
│   ├── main.cpp
│   ├── mqtt_handler.h
│   └── mqtt_handler.cpp
├── test/
└── .vscode/
```
