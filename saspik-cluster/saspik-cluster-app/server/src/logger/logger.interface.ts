/**
 * Единый JSON-конверт лога (согласован для device / server / rule-engine).
 * В Influx: теги = topic, level, event, src; поля = msg и др.
 */
export interface LogEnvelope {
  level: "debug" | "info" | "warn" | "error";
  src: string;
  event: string;
  msg: string;
  topic?: string;
  [key: string]: unknown;
}

export interface ILogger {
  log: (...args: unknown[]) => void;

  error: (...args: unknown[]) => void;

  warn: (...args: unknown[]) => void;
}
