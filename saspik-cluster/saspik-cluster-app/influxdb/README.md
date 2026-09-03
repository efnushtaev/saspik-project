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

Бакет `logs` с retention 7 дней создаётся init-скриптом `init/create-logs-bucket.sh`,
смонтированным в `/docker-entrypoint-initdb.d` (выполняется один раз при первой инициализации).

Script использует `influx bucket create --retention <N>d` и CLI `influx`, ожидает готовности API (`influx ping`).

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
