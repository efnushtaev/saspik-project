[↑ Saspik-cluster](../README.md)

# InfluxDB — бакеты и retention

InfluxDB 2.x (образ `influxdb:2`) хранит данные от Telegraf.

## Бакеты

| Бакет | ENV (default) | Retention | Содержимое |
|---|---|---|---|
| Основной | `INFLUXDB_BUCKET` (напр. `mqtt`) | задаётся `DOCKER_INFLUXDB_INIT_*` | сенсорные/командные данные (measurement `mqtt_consumer`) |
| Логи | `INFLUXDB_LOGS_BUCKET` (`logs`) | `INFLUXDB_LOGS_RETENTION_DAYS` (`7`) | логи-конверты device/server/rule-engine (measurement `logs`) |

## Инициализация

Основной бакет/пользователь создаются через env контейнера `DOCKER_INFLUXDB_INIT_*` при первом старте.

Бакет `logs` с retention 7 дней создаётся двумя путями:

1. **init-скрипт** `init/create-logs-bucket.sh`, смонтированный в `/docker-entrypoint-initdb.d` (выполняется один раз при первой инициализации volume `influxdb-data`).
2. **Сервис `ensure-logs-bucket`** (`docker-compose.yml`) — одноразовый job на тот случай, если volume уже инициализирован и init-скрипт больше не сработает (например, при добавлении фичи на существующем деплое). Идемпотентен: если бакет есть — просто выходит с кодом 0.

Оба используют один и тот же скрипт; адрес API задаётся `INFLUXDB_HOST` (внутри контейнера influxdb — `http://localhost:8086`, из `ensure-logs-bucket` — `http://influxdb:8086`). Запуск на уже развёрнутом стенде:

```bash
docker compose up -d ensure-logs-bucket
```

## Переменные окружения инфлюкс-контейнера

```env
DOCKER_INFLUXDB_INIT_MODE=setup
DOCKER_INFLUXDB_INIT_USERNAME=admin
DOCKER_INFLUXDB_INIT_PASSWORD=admin123
DOCKER_INFLUXDB_INIT_ORG=${INFLUXDB_ORG}
DOCKER_INFLUXDB_INIT_BUCKET=${INFLUXDB_BUCKET}
DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=${INFLUXDB_TOKEN}
INFLUXDB_LOGS_BUCKET=${INFLUXDB_LOGS_BUCKET:-logs}
INFLUXDB_LOGS_RETENTION_DAYS=${INFLUXDB_LOGS_RETENTION_DAYS:-7}
```
