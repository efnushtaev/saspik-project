# Классификация IoT-устройств САСПИК

## Формат

```
saspik.{роли}.{протокол}.{питание}{nnn}
```

## Коды

### Роли (можно комбинировать)
| Код | Значение |
|-----|----------|
| `g` | Gateway (шлюз) |
| `s` | Sensor (сенсор) |
| `a` | Actuator (исполнительное устройство) |

### Протоколы (можно комбинировать)
| Код | Значение |
|-----|----------|
| `wm` | WiFi + MQTT |
| `en` | ESP-NOW |
| `lr` | LoRa |

### Питание (можно комбинировать)
| Код | Значение |
|-----|----------|
| `m` | mains (сетевое) |
| `b` | battery (батарейное) |

### Номер
Трёхзначный серийный номер: `001`, `002`… `000` — шаблон.

## Примеры

| Класс | Устройство |
|-------|-----------|
| `saspik.g.wm.m001` | esp32-controller (шлюз ESP-NOW → MQTT + реле) |
| `saspik.sa.wm.m001` | esp32-local-mqtt (DHT22 + LED, WiFi+MQTT) |
| `saspik.s.en.b001` | saspik-iot-node-sensor-1 (DHT22 + float sensor, ESP-NOW, battery) |
| `saspik.s.en.b002` | esp32-battery-check (DHT22, ESP-NOW, battery) |
| `saspik.s.en.b000` | esp32-node-template (шаблон) |
