#!/bin/bash
# Обёртка entrypoint контейнера influxdb.
# Запускает штатный /entrypoint.sh (setup на свежем volume + настоящий influxd),
# дожидается готовности API и идемпотентно создаёт бакеты (ensure-buckets.sh).
# Выполняется на КАЖДОМ старте контейнера, поэтому бакеты гарантированно есть
# и на уже инициализированном volume (где /docker-entrypoint-initdb.d не сработает).

set -euo pipefail

PROVISION_DIR="$(dirname "$(readlink -f "$0")")"

# Штатный entrypoint: на свежем volume выполняет influx setup, затем exec'ит influxd.
/entrypoint.sh influxd &
INFLUXD_PID=$!

forward_signal() {
  kill -"$1" "${INFLUXD_PID}" 2>/dev/null || true
  wait "${INFLUXD_PID}" 2>/dev/null || true
}
trap 'forward_signal TERM' TERM
trap 'forward_signal INT' INT
trap 'trap - EXIT TERM INT; wait "${INFLUXD_PID}" 2>/dev/null || true' EXIT

# Ждём настоящий influxd на боевом адресе: во время setup временный инстанс
# сидит на INFLUXD_INIT_PORT (9999), а 8086 поднимается только после него.
max_wait=180
until curl -sf "http://localhost:8086/health" >/dev/null 2>&1; do
  if [ "${max_wait}" -le 0 ]; then
    echo "bootstrap: influxd не поднялся на :8086"
    exit 1
  fi
  max_wait=$((max_wait - 1))
  sleep 1
done

# Создание бакетов с ретраями — накрывает короткое окно рестарта после setup.
for attempt in 1 2 3 4 5; do
  if bash "${PROVISION_DIR}/ensure-buckets.sh" "${@}"; then
    break
  fi
  echo "bootstrap: ensure-buckets (попытка ${attempt}) не удалась, повтор через 5с"
  if [ "${attempt}" -ge 5 ]; then
    echo "bootstrap: не удалось создать бакеты"
    exit 1
  fi
  sleep 5
done

wait "${INFLUXD_PID}"