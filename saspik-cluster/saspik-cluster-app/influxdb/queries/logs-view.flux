// ============================================================================
// Просмотр логов (measurement "logs", бакет "logs").
// Вставляется в Data Explorer InfluxDB (Script Editor) — см. telegraf/README.md.
//
// Почему не работает дефолтный запрос Data Explorer: он агрегирует
// aggregateWindow(every: ..., fn: mean), а mean() во Flux работает только
// с числами. Поля логов (msg, cause) — строки, поэтому используем last/limit.
// ============================================================================

// --- Сырые логи: последние 200 сообщений за выбранный период ---
from(bucket: "logs")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "logs")
  |> filter(fn: (r) => r._field == "msg")
  |> limit(n: 200)
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> sort(columns: ["_time"], desc: true)
  |> yield(name: "logs_raw")

// --- Последний лог по каждому src/topic за 24 часа ---
from(bucket: "logs")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "logs")
  |> filter(fn: (r) => r._field == "msg")
  |> group(columns: ["src", "topic"])
  |> last()
  |> yield(name: "logs_last")