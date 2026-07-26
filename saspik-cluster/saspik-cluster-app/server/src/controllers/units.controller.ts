import { Response, Request } from "express";
import { inject, injectable } from "inversify";

import "reflect-metadata";
import {
  GetUnitsListResponse,
  IUnitsController,
} from "./units.controller.interface";
import { BaseController } from "../common/baseController";
import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { UnitsControllersRoutesURL, RequestMethod } from "../const";
import { IUnitsService } from "../services/units";

@injectable()
export class UnitsController
  extends BaseController
  implements IUnitsController
{
  constructor(
    @inject(TYPES.Logger) private loggerService: ILogger,
    @inject(TYPES.UnitsService)
    private unitsService: IUnitsService,
  ) {
    super(loggerService);
    this.bindRoutes([
      {
        path: UnitsControllersRoutesURL.UNITS_LIST,
        method: RequestMethod.GET,
        func: this.getUnitsList,
      },
    ]);
  }

  async getUnitsList(_: Request, res: Response) {
    const units = await this.unitsService.getUnits();

    return this.ok<GetUnitsListResponse>(res, { units });
  }
}
