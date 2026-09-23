[↑ Saspik-cluster](../README.md)

# ⧫ docs/logs

#### Логирование и диагностика кластера: единый JSON-конверт для всех компонентов, лог-топики и хранение в InfluxDB.

### Формат

Все компоненты (device, server, rule-engine) публикуют логи в **едином JSON-конверте** в свои лог-топики. Telegraf отводит их в отдельный measurement `logs` в InfluxDB (бакет `logs`, retention 7 дней).

##### Лог-топики

| Топик | Источник |
| :--- | :--- |
| `device/{unitId}/{objectId}/log` | ESP32 (device-log): старт/ребут, connect/disconnect, диагностика, хвост лога |
| `server/worker/log` | Backend (Express): connect/disconnect/ошибки MQTT |
| `rule-engine/worker/log` | MQTT Rule Engine: срабатывание/ошибки правил, connect/disconnect |

##### Схема конверта

```json
{
  "level":    "info",      // debug | info | warn | error
  "src":      "device",    // device | server | rule-engine
  "event":    "startup",   // startup|reboot|connect|disconnect|diag|rule-fired|rule-error|...
  "msg":      "человекочитаемый текст",
  "topic":    "device/unitId2/saspik.sa.wm.m001/log",
  "unitId":   "unitId2",   // device
  "objectId": "saspik.sa.wm.m001", // device
  "uptime":   12345,       // device, сек
  "cause":    "wifi-lost"  // device, при ребуте (wifi-lost | mqtt-timeout)
}
```

- **device**: при старте публикует `event=startup` с хвостом лога в `msg` и причиной ребута в `cause`; при обрыве — `event=diag` с `msg`-сводкой и полями `wifi`/`mqtt`/`lostSec`; `event=reboot` перед перезапуском.
- **rule-engine**: при срабатывании правила — `event=rule-fired`, при ошибке — `event=rule-error`.
- **server**: при connect/disconnect/ошибке брокера — `event=connect|disconnect|mqtt-error`.

### Хранение в InfluxDB

- **Measurement**: `logs` (через `name_override="logs"` в telegraf).
- **Теги**: `topic`, `level`, `event`, `src`, `unitId`, `objectId`.
- **Поля**: `msg`, `cause`, `uptime` и др.
- **Бакет**: `logs` (`INFLUXDB_LOGS_BUCKET`), retention **7 дней** (`INFLUXDB_LOGS_RETENTION_DAYS`).
- Сенсорные данные остаются в measurement `mqtt_consumer` (бакет `mqtt`).

Пример Flux-запроса по логам уровня `error`:

```flux
from(bucket: "logs")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "logs")
  |> filter(fn: (r) => r.level == "error")
  |> filter(fn: (r) => r._field == "msg")
  |> last()
```

### Против спама

- Mosquitto: `log_type notice` (без поштучного debug).
- Rule Engine: не логирует каждое входящее сообщение — только срабатывания и ошибки правил.
- Device: пишет только события (старт/ребут/connect/диагностика/обрыв), а не каждое чтение сенсора.