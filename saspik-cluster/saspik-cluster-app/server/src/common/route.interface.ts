import { NextFunction, Request, Response, Router } from "express";

import { IMiddleware } from "./middleware.interface";
import { RequestMethod } from "../const";

export interface IControllerRoute<Req> {
  path: string;
  func: (req: Request<Req>, res: Response, next: NextFunction) => void;
  method: keyof Pick<Router, RequestMethod>;
  middlewares?: IMiddleware<Req>[];
}
