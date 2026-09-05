[↑ Saspik-cluster](../README.md)

# Telegraf — запись MQTT → InfluxDB

Подписывается на MQTT-топики кластера (mosquitto) и пишет в InfluxDB 2.x.

## Конфигурация (`telegraf.conf`)

Два входа `mqtt_consumer` и два выхода `outputs.influxdb_v2` (разделение по `namepass`):

| Вход | Топики | Measurement | Бакет (выход) |
|---|---|---|---|
| Данные (`data_format=json`) | `sensor/#`, `units/#`, `led/#` | `mqtt_consumer` | `${INFLUXDB_BUCKET}` (напр. `mqtt`) |
| Логи (`data_format=json`) | `device/+/+/log`, `server/+/log`, `rule-engine/+/log` | `logs` (`name_override`) | `${INFLUXDB_LOGS_BUCKET}` (напр. `logs`) |

> Вход «Данные» **намеренно не подписан** на `healthcheck/#`: healthcheck mosquitto
> публикует в `healthcheck/ping` payload каждые 30 с, и не-JSON payload (`test`)
> ломал парсер `data_format=json`, из-за чего падал **весь батч** `mqtt_consumer`
> и новые данные не доходили до InfluxDB (см. раздел «Известные проблемы» ниже).

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

## Известные проблемы

### `healthcheck/ping` ломал JSON-парсер (09.2026)

**Симптом:** в логах telegraf ошибка
`[inputs.mqtt_consumer] Error in plugin: invalid character 'e' in literal true (expecting 'r')`,
и **новые данные не появлялись** в InfluxDB (ни сенсорные, ни логи) — падал весь батч `mqtt_consumer`.

**Причина:** healthcheck контейнера mosquitto каждые 30 с публикует в `healthcheck/ping`
payload `test` (не JSON). Telegraf был подписан на `healthcheck/#` во входе с `data_format=json`,
поэтому каждый healthcheck-тик ронял парсер и запись.

**Решение:**
1. `healthcheck/#` убран из JSON-входа telegraf (`sensor/#`, `units/#`, `led/#` остались).
2. Payload healthcheck mosquitto в `docker-compose.yml` заменён на валидный JSON:
   `-m "test"` → `-m '{"payload":"test"}'` (семантически корректный формат на будущее —
   даже если топик снова подпишут на JSON-вход, он не сломает парсер).

После правок — пересобрать и перезапустить:
```bash
docker compose build telegraf
docker compose up -d telegraf mosquitto
```
