import { injectable } from "inversify";

import { ILogger, LogEnvelope } from "./logger.interface";
import { buildEnvelope } from "./log-envelope";
import "reflect-metadata";

/**
 * Единый структурированный логгер сервера.
 *
 * Выводит в stdout JSON-конверт (см. LogEnvelope), согласованный для
 * device / server / rule-engine. Схема и теги в Influx описаны в docs/mqtt-topics.md.
 */
@injectable()
export class LoggerService implements ILogger {
  private readonly src = "server";

  log(...args: unknown[]) {
    this.emit("info", args);
  }

  error(...args: unknown[]) {
    this.emit("error", args);
  }

  warn(...args: unknown[]) {
    this.emit("warn", args);
  }

  /**
   * Формирует и выводит JSON-конверт лога.
   */
  private emit(level: LogEnvelope["level"], args: unknown[]): void {
    const line = JSON.stringify(buildEnvelope(this.src, level, args));
    if (level === "error") {
      console.error(line);
    } else if (level === "warn") {
      console.warn(line);
    } else {
      console.log(line);
    }
  }
}
