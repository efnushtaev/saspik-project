import { LogEnvelope } from "./logger.interface";

/**
 * Разбирает аргументы логгера и собирает плоский msg:
 * - строковые аргументы склеиваются пробелом;
 * - объекты/числа сериализуются в JSON и добавляются в msg хвостом.
 */
function formatArgs(args: unknown[]): string {
  return args
    .map((a) => {
      if (typeof a === "string") return a;
      if (typeof a === "number" || typeof a === "boolean") return String(a);
      try {
        return JSON.stringify(a);
      } catch {
        return String(a);
      }
    })
    .join(" ");
}

/**
 * Формирует единый JSON-конверт лога сервера.
 * @param src - компонент-источник (обычно "server").
 * @param level - уровень логирования.
 * @param args - аргументы логгера (префикс-строка, опционально объект).
 */
export function buildEnvelope(
  src: string,
  level: LogEnvelope["level"],
  args: unknown[],
): LogEnvelope {
  // Строковый префикс вида "[ClassName] message" — берём как msg целиком,
  // возможность получать event из системных префиксов опускаем (простота).
  const msg = formatArgs(args);

  return {
    level,
    src,
    event: "log",
    msg,
  };
}
