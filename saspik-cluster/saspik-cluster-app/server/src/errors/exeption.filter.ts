import { NextFunction, Request, Response } from "express";
import { inject, injectable } from "inversify";

import { TYPES } from "../types";
import { IExeptionFilter } from "./exeption.filter.interface";
import { HTTPError } from "./http-error.class";
import "reflect-metadata";
import { ILogger } from "../logger/logger.interface";

@injectable()
export class ExeptionFilter implements IExeptionFilter {
  constructor(@inject(TYPES.Logger) private logger: ILogger) {}

  catch(
    err: Error | HTTPError,
    _req: Request,
    res: Response,
    // eslint-disable-next-line @typescript-eslint/no-unused-vars
    _next: NextFunction,
  ) {
    if (err instanceof HTTPError) {
      this.logger.error(
        `[${err.context}] Ошибка ${err.statusCode}: ${err.message}`,
      );
      res.status(err.statusCode).send({ err: err.message });
    } else {
      this.logger.error("Получена ошибка: ", err.message);
      res.status(500).send({ err: `Получена ошибка: ${err.message}` });
    }
  }
}
