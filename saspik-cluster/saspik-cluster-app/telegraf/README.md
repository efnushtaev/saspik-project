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

Бакет `logs` создаётся на каждом старте InfluxDB скриптом `influxdb/provision/ensure-buckets.sh`
с retention `INFLUXDB_LOGS_RETENTION_DAYS` (по умолчанию 7 дней) — см. `influxdb/README.md`.
Скрипт вшит в образ InfluxDB (см. `influxdb/Dockerfile`).

### Просмотр логов в Data Explorer

Ошибка `unsupported input type for mean aggregate: string` при выборе бакета `logs`
в Data Explorer InfluxDB (`:8086`) — **не баг пайплайна**: дефолтный запрос UI
применяет `aggregateWindow(every: ..., fn: mean)`, а `mean()` во Flux работает только
с числами. Поля логов (`msg`, `cause`) — строки (`json_string_fields`), поэтому
агрегировать их средним нельзя. Бакет `mqtt` не падает, потому что там дефолтное
поле числовое (`value`).

Смотреть логи без ошибки можно двумя способами:

- **Сменить агрегацию** в Query Builder: вместо `mean` выбрать `last` или `count`
  (подходят и для строковых полей).
- **Вставить готовый скрипт** в Script Editor — см. `influxdb/queries/logs-view.flux`
  (сырые логи `limit(n: 200)` или last по `src`/`topic`). Вид — Table / Logs.

Ошибки в коде нет — её генерирует только Data Explorer; сервис чтения состояния
(`influxDbStateStore.service.ts`) использует `last()`, а не `mean()`.

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
