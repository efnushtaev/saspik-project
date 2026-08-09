import { Response, Request } from "express";
import { inject, injectable } from "inversify";

import "reflect-metadata";
import {
  GetUnitsListResponse,
  GetUnitResponse,
  CreateUnitResponse,
  UpdateUnitResponse,
  DeleteUnitResponse,
  IUnitsController,
} from "./units.controller.interface";
import { BaseController } from "../common/baseController";
import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { UnitsControllersRoutesURL, RequestMethod } from "../const";
import { IUnitsService } from "../services/units";
import { CreateUnitDto, UpdateUnitDto } from "../dto/units.dto";

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
      {
        path: UnitsControllersRoutesURL.UNITS_GET_BY_ID,
        method: RequestMethod.GET,
        func: this.getUnitById,
      },
      {
        path: UnitsControllersRoutesURL.UNITS_CREATE,
        method: RequestMethod.POST,
        func: this.createUnit,
      },
      {
        path: UnitsControllersRoutesURL.UNITS_UPDATE,
        method: RequestMethod.PATCH,
        func: this.updateUnit,
      },
      {
        path: UnitsControllersRoutesURL.UNITS_DELETE,
        method: RequestMethod.DELETE,
        func: this.deleteUnit,
      },
    ]);
  }

  async getUnitsList(_: Request, res: Response) {
    const units = await this.unitsService.getUnits();

    return this.ok<GetUnitsListResponse>(res, { units });
  }

  async getUnitById({ params }: Request<{ id: string }>, res: Response) {
    const unit = await this.unitsService.getUnitById(params.id);
    if (!unit) {
      return this.send(res, 404, { error: "Unit not found" });
    }
    return this.ok<GetUnitResponse>(res, { unit });
  }

  async createUnit(
    { body }: Request<never, never, CreateUnitDto>,
    res: Response,
  ) {
    try {
      const unit = await this.unitsService.createUnit(body);
      return this.ok<CreateUnitResponse>(res, { unit });
    } catch (error) {
      return this.send(res, 400, {
        error: error instanceof Error ? error.message : "Unknown error",
      });
    }
  }

  async updateUnit(
    { params, body }: Request<{ id: string }, never, UpdateUnitDto>,
    res: Response,
  ) {
    try {
      const unit = await this.unitsService.updateUnit(params.id, body);
      if (!unit) {
        return this.send(res, 404, { error: "Unit not found" });
      }
      return this.ok<UpdateUnitResponse>(res, { unit });
    } catch (error) {
      return this.send(res, 400, {
        error: error instanceof Error ? error.message : "Unknown error",
      });
    }
  }

  async deleteUnit({ params }: Request<{ id: string }>, res: Response) {
    const deleted = await this.unitsService.deleteUnit(params.id);
    if (!deleted) {
      return this.send(res, 404, { error: "Unit not found" });
    }
    return this.ok<DeleteUnitResponse>(res, { success: true });
  }
}
