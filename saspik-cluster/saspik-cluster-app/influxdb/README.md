[↑ Saspik-cluster](../README.md)

# ⧫ InfluxDB — бакеты и retention

#### Time-series база данных (InfluxDB 2.x) для сенсорных данных и логов

InfluxDB 2.x хранит данные от Telegraf.
Образ собирается локально из `influxdb/Dockerfile` (`FROM influxdb:2.9.1`).
В образ вшит каталог `provision/` — никаких bind-mount'ов скриптов в compose, они всегда доступны
внутри образа по пути `/provision/` (важно для деплоя через Portainer из Git,
где bind-mount на недостающий каталог на сервере падал бы).

### Бакеты

| Бакет | ENV (default) | Retention | Содержимое |
| :--- | :--- | :--- | :--- |
| Основной | `INFLUXDB_BUCKET` (напр. `mqtt`) | задаётся `DOCKER_INFLUXDB_INIT_*` | сенсорные/командные данные (measurement `mqtt_consumer`) |
| Логи | `INFLUXDB_LOGS_BUCKET` (`logs`) | `INFLUXDB_LOGS_RETENTION_DAYS` (`7`) | логи-конверты device/server/rule-engine (measurement `logs`) |

### Инициализация

Основной бакет/пользователь создаются через env контейнера `DOCKER_INFLUXDB_INIT_*` при первом старте.

Бакет `logs` с retention 7 дней создаётся вместе с основным бакетом на **каждом
старте контейнера** обёрткой `provision/bootstrap.sh` (лежит в образе):

- `bootstrap.sh` — entrypoint-обёртка: запускает штатный `/entrypoint.sh influxd`
  (на свежем volume он выполняет setup и создаёт org/токен/бакет `mqtt`),
  дожидается API на `:8086` (до 300 с) и вызывает `ensure-buckets.sh`.
- `ensure-buckets.sh` — идемпотентно создаёт бакеты из `INFLUXDB_BUCKETS`
  (по умолчанию `mqtt,logs`), если их нет. `logs` — с retention
  `INFLUXDB_LOGS_RETENTION_DAYS` (по умолчанию 7 дней), `mqtt` — без retention.
  Токен/org берёт из `INFLUXDB_TOKEN`/`INFLUXDB_ORG`, при их отсутствии —
  из `DOCKER_INFLUXDB_INIT_ADMIN_TOKEN`/`DOCKER_INFLUXDB_INIT_ORG`.

Provisioning **нефатален**: даже если бакеты не создались, influxd продолжает
работать на `:8086` (в логах предупреждение) — provisioning не может уронить
доступность InfluxDB.

Благодаря этому бакеты гарантированно существуют и на уже инициализированном
volume (штатный `/docker-entrypoint-initdb.d` там не выполнялся бы).
Setup через env `DOCKER_INFLUXDB_INIT_*` остаётся для свежего volume — он же
создаёт служебные бакеты `_monitoring` и `_tasks`.

### Переменные окружения контейнера

```env
DOCKER_INFLUXDB_INIT_MODE=setup
DOCKER_INFLUXDB_INIT_USERNAME=admin
DOCKER_INFLUXDB_INIT_PASSWORD=admin123
DOCKER_INFLUXDB_INIT_ORG=${INFLUXDB_ORG}
DOCKER_INFLUXDB_INIT_BUCKET=${INFLUXDB_BUCKET}
DOCKER_INFLUXDB_INIT_ADMIN_TOKEN=${INFLUXDB_TOKEN}
INFLUXDB_BUCKETS=${INFLUXDB_BUCKETS:-mqtt,logs}
INFLUXDB_LOGS_RETENTION_DAYS=${INFLUXDB_LOGS_RETENTION_DAYS:-7}
```
