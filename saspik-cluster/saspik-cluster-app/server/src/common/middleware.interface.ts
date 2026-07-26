import { Request, Response, NextFunction } from "express";

import { TEMPORARY_ANY } from "../types";

export interface IMiddleware<Req = TEMPORARY_ANY> {
  execute(req: Request<Req>, res: Response, next: NextFunction): void;
}
