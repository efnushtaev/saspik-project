[↑ Saspik-iot](README.md)

# ⧫ Реестр устройств

#### Таблица созданных iot-устройств: objectId, прошивка, тип, описание
Единый реестр для проверки занятости objectId и unitId при создании новых скетчей.
Заполняется при создании прошивки (скилл `firmware-gen`); boilerplate в реестр не входит — это основа, а не устройство.

### Устройства

| `objectId` | Прошивка | Тип | Описание |
| :--- | :--- | :--- | :--- |
| `saspik-gwqm001` | [esp32-controller](saspik-iot-sketches/esp32-controller/README.md) | шлюз | Шлюз ESP-NOW → MQTT, сбор данных от узлов, реле |
| `saspik.sa.wm.m003` | [esp32-local-mqtt](saspik-iot-sketches/esp32-local-mqtt/README.md) | сенсор | DHT22 + LED, WiFi+MQTT, устойчивость к сбоям (авто-ребут, лог причины ребута) |
| `saspik.s.en.b001` | [saspik-iot-node-sensor-1](saspik-iot-sketches/saspik-iot-node-sensor-1/README.md) | сенсор | DHT22 + геркон, ESP-NOW, battery |
| `saspik.s.en.b002` | [esp32-battery-check](saspik-iot-sketches/esp32-battery-check/README.md) | сенсор | DHT22, ESP-NOW, battery |
| `saspik.s.en.b000` | [esp32-node-template](saspik-iot-sketches/esp32-node-template/README.md) | шаблон | Шаблон узла ESP-NOW (`nnn=000` — шаблон по классификатору) |

### Замечания

- **`esp32-local-mqtt`**: фактический objectId `saspik.sa.wm.m003` — старый
  формат (точки, `wm`), классификатор требует `saspik-sawqm…`
  (см. [classification](docs/classification.md)). В реестре — фактическое
  значение (по решению), переименование не выполнялось.
- Формат objectId нового устройства: `saspik-{роли}{протокол}{питание}{nnn}` —
  см. [classification](docs/classification.md).
- Файл намеренно лежит вне `docs/` — не краулится `docs-gen`.
