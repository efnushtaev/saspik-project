#!/bin/bash
# Обёртка entrypoint контейнера influxdb.
# Запускает штатный /entrypoint.sh (setup на свежем volume + настоящий influxd),
# дожидается готовности API и идемпотентно создаёт бакеты (ensure-buckets.sh).
# Выполняется на КАЖДОМ старте контейнера, поэтому бакеты гарантированно есть
# и на уже инициализированном volume (где /docker-entrypoint-initdb.d не сработает).
#
# Provisioning НЕФАТАЛЕН: если бакеты создать не удалось, influxd всё равно
# остаётся поднятым (логируем и продолжаем), чтобы provisioning не мог уронить
# доступность InfluxDB на :8086.

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
max_wait=300
until curl -sf "http://localhost:8086/health" >/dev/null 2>&1; do
  if [ "${max_wait}" -le 0 ]; then
    echo "bootstrap: influxd не поднялся на :8086 за отведённое время (300с), продолжаем наблюдение"
    break
  fi
  max_wait=$((max_wait - 1))
  sleep 1
done

# Создание бакетов с ретраями — накрывает короткое окно рестарта после setup.
# Неудача provisioning НЕ останавливает influxd: только логируем.
bucket_ok=1
for attempt in 1 2 3 4 5; do
  if bash "${PROVISION_DIR}/ensure-buckets.sh" "${@}"; then
    bucket_ok=0
    break
  fi
  echo "bootstrap: ensure-buckets (попытка ${attempt}) не удалась, повтор через 5с"
  sleep 5
done

if [ "${bucket_ok}" -ne 0 ]; then
  echo "bootstrap: ВНИМАНИЕ — не удалось создать бакеты, influxd продолжает работать. Создайте бакеты вручную (см. influxdb/README.md)."
fi

wait "${INFLUXD_PID}"