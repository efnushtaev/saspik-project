import { config, DotenvConfigOutput, DotenvParseOutput } from "dotenv";
import { inject, injectable } from "inversify";
import path from "path";

import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { IConfigService } from "./config.service.interface";

@injectable()
export class ConfigService implements IConfigService {
  private config: DotenvParseOutput | NodeJS.ProcessEnv;

  constructor(@inject(TYPES.Logger) private logger: ILogger) {
    // Always start with process.env as the base configuration
    this.config = { ...process.env };

    // In non-production environments, try to load .env file
    if (process.env.NODE_ENV !== "prod") {
      const result: DotenvConfigOutput = config({ path: path.resolve(__dirname, "../../../.env") });

      if (result.error) {
        this.logger.error(
          "[ConfigService] Не удалось прочитать файл .env или он отсутствует",
        );
      } else {
        this.logger.log("[ConfigService] Конфигурация .env загружена");
        // Merge .env file variables with process.env (env file variables won't override existing env vars)
        this.config = { ...this.config, ...result.parsed };
      }
    } else {
      this.logger.log(
        "[ConfigService] Конфигурация загружена из переменных окружения",
      );
    }
  }


  get(key: string): string | undefined {
    return this.config[key];
  }
}
