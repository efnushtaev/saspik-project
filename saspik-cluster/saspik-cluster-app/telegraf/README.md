[↑ Saspik-cluster](../README.md)

# Telegraf — запись MQTT → InfluxDB

Подписывается на MQTT-топики кластера (mosquitto) и пишет в InfluxDB 2.x.

## Конфигурация (`telegraf.conf`)

Два входа `mqtt_consumer` и два выхода `outputs.influxdb_v2` (разделение по `namepass`):

| Вход | Топики | Measurement | Бакет (выход) |
|---|---|---|---|
| Данные (`data_format=json`) | `sensor/#`, `units/#`, `led/#`, `healthcheck/#` | `mqtt_consumer` | `${INFLUXDB_BUCKET}` (напр. `mqtt`) |
| Логи (`data_format=json`) | `device/+/+/log`, `server/+/log`, `rule-engine/+/log` | `logs` (`name_override`) | `${INFLUXDB_LOGS_BUCKET}` (напр. `logs`) |

Выходы разделяют потоки через `namepass`:
- `namepass = ["mqtt_consumer"]` → только сенсорные/командные данные в основной бакет;
- `namepass = ["logs"]` → только логи в отдельный бакет `logs`.

### Логи (measurement `logs`)

Теги: `topic`, `level`, `event`, `src`, `unitId`, `objectId`.
Поля: `msg`, `cause`, `uptime` и др. (`json_string_fields = ["msg","cause"]`).

Бакет `logs` создаётся при инициализации InfluxDB с retention `INFLUXDB_LOGS_RETENTION_DAYS` (по умолчанию 7 дней) — см. `influxdb/init/create-logs-bucket.sh`.

## Переменные окружения

- `INFLUXDB_TOKEN`, `INFLUXDB_ORG`, `INFLUXDB_BUCKET` — основной бакет
- `INFLUXDB_LOGS_BUCKET` (default `logs`), `INFLUXDB_LOGS_RETENTION_DAYS` (default `7`) — бакет логов

## Сборка и запуск

```bash
docker compose build telegraf
docker compose up -d telegraf
```
