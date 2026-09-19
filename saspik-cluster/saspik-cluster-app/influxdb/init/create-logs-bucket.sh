#!/usr/bin/env bash
# Создаёт бакет логов с retention 7 дней для данных телеметрии/диагностики.
# Используется штатным init-механизмом InfluxDB (docker-entrypoint-initdb.d —
# выполняется один раз при инициализации volume) и одноразовым сервисом
# ensure-logs-bucket (docker-compose.yml) для уже инициализированных volume.
#
# INFLUXDB_TOKEN / INFLUXDB_ORG / INFLUXDB_LOGS_BUCKET / INFLUXDB_HOST
# передаются через env контейнера influxdb (см. docker-compose.yml).

: "${INFLUXDB_TOKEN:?INFLUXDB_TOKEN required}"
: "${INFLUXDB_ORG:?INFLUXDB_ORG required}"
LOGS_BUCKET="${INFLUXDB_LOGS_BUCKET:-logs}"
RETENTION_DAYS="${INFLUXDB_LOGS_RETENTION_DAYS:-7}"
# Адрес API InfluxDB. Внутри самого контейнера influxdb работает localhost,
# из отдельного сервиса (ensure-logs-bucket) передаётся http://influxdb:8086.
INFLUXDB_HOST="${INFLUXDB_HOST:-http://localhost:8086}"

# Ждём готовности InfluxDB API (до первых секунд может быть недоступен).
# /health не требует токена, поэтому ping вызывается без --token.
max_wait=30
until influx ping --host "${INFLUXDB_HOST}" 2>/dev/null; do
  if [ "$max_wait" -le 0 ]; then
    echo "logs-bucket: InfluxDB не поднялся за отведённое время"
    exit 1
  fi
  max_wait=$((max_wait - 1))
  sleep 1
done

# Создать бакет, если его ещё нет
if ! influx bucket list --host "${INFLUXDB_HOST}" --token "${INFLUXDB_TOKEN}" --org "${INFLUXDB_ORG}" --name "${LOGS_BUCKET}" 2>/dev/null | grep -q "${LOGS_BUCKET}"; then
  influx bucket create \
    --host "${INFLUXDB_HOST}" \
    --token "${INFLUXDB_TOKEN}" \
    --org "${INFLUXDB_ORG}" \
    --name "${LOGS_BUCKET}" \
    --retention "${RETENTION_DAYS}d"
  echo "logs-bucket: создан бакет ${LOGS_BUCKET} (retention ${RETENTION_DAYS}д)"
else
  echo "logs-bucket: бакет ${LOGS_BUCKET} уже существует, пропуск"
fi
