#!/bin/bash
# Идемпотентно создаёт бакеты, если их ещё нет. Запускается обёрткой
# bootstrap.sh на каждом старте контейнера influxdb, поэтому гарантирует
# наличие нужных бакетов и на уже инициализированном volume.
#
# Env:
#   INFLUXDB_TOKEN        — admin-токен (обязателен)
#   INFLUXDB_ORG          — организация (обязательна)
#   INFLUXDB_BUCKETS      — список бакетов через запятую (default: mqtt,logs)
#   INFLUXDB_LOGS_RETENTION_DAYS — retention для бакета logs (default: 7)

set -euo pipefail

: "${INFLUXDB_TOKEN:?INFLUXDB_TOKEN required}"
: "${INFLUXDB_ORG:?INFLUXDB_ORG required}"

HOST="${INFLUXDB_HOST:-http://localhost:8086}"
BUCKETS="${INFLUXDB_BUCKETS:-mqtt,logs}"
LOGS_RETENTION="${INFLUXDB_LOGS_RETENTION_DAYS:-7}"

# Ждём готовности InfluxDB API. /health не требует токена.
max_wait=180
until curl -sf "${HOST}/health" >/dev/null 2>&1; do
  if [ "${max_wait}" -le 0 ]; then
    echo "ensure-buckets: InfluxDB не поднялся за отведённое время"
    exit 1
  fi
  max_wait=$((max_wait - 1))
  sleep 1
done

IFS=',' read -ra NAMES <<<"${BUCKETS}"
for bucket in "${NAMES[@]}"; do
  if influx bucket list --host "${HOST}" --token "${INFLUXDB_TOKEN}" --org "${INFLUXDB_ORG}" --name "${bucket}" 2>/dev/null | grep -q "${bucket}"; then
    echo "ensure-buckets: бакет ${bucket} уже существует, пропуск"
    continue
  fi

  if [ "${bucket}" = "logs" ]; then
    influx bucket create \
      --host "${HOST}" \
      --token "${INFLUXDB_TOKEN}" \
      --org "${INFLUXDB_ORG}" \
      --name "${bucket}" \
      --retention "${LOGS_RETENTION}d"
    echo "ensure-buckets: создан бакет ${bucket} (retention ${LOGS_RETENTION}д)"
  else
    influx bucket create \
      --host "${HOST}" \
      --token "${INFLUXDB_TOKEN}" \
      --org "${INFLUXDB_ORG}" \
      --name "${bucket}"
    echo "ensure-buckets: создан бакет ${bucket}"
  fi
done