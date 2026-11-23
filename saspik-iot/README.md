[↑ Назад](../README.md)

# ⧫ Saspik-iot

#### ESP32 прошивки для IoT-устройств: шлюз, сенсорные узлы (ESP-NOW, MQTT, deep sleep)

Контекст проекта: `opencode/agents.md → saspik-iot`.
Скетчи живут в `Saspik-iot-sketches/`.

### Программное обеспечение

- [`esp32-boilerplate`](saspik-iot-sketches/esp32-boilerplate/README.md) — основа для новых прошивок ESP32: `saspik-core`, шаблоны Gateway/Sensor/Device, host-тесты

### Классификация устройств

Устройства классифицируются по формату `saspik-{роли}{протокол}{питание}{nnn}`.
Подробнее: [`docs/classification.md`](docs/classification.md)

### Индикация состояния связи

Светодиод как единственный канал диагностики, когда MQTT недоступен: что означает
мигание, как настроить пин и как индикация сосуществует с каналом `led`.
Подробнее: [`docs/indication.md`](docs/indication.md)

### Реестр устройств

Таблица созданных устройств (objectId, прошивка, тип, описание): [`devices.md`](devices.md)