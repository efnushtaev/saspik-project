import { NextFunction, Response, Request } from "express";

import { UnitDto, CreateUnitDto, UpdateUnitDto } from "../dto/units.dto";
import { ControllerResponseMessage } from "../common/controller.types";

export type GetUnitsListResponse = { units: UnitDto[] };
export type GetUnitResponse = { unit: UnitDto };
export type CreateUnitResponse = { unit: UnitDto };
export type UpdateUnitResponse = { unit: UnitDto };
export type DeleteUnitResponse = { success: boolean };

export type UnitParamsReq = {
  id: string;
};

export interface IUnitsController {
  getUnitsList: (
    req: Request,
    res: Response,
    next: NextFunction,
  ) => ControllerResponseMessage<GetUnitsListResponse>;

  getUnitById: (
    req: Request<UnitParamsReq>,
    res: Response,
  ) => ControllerResponseMessage<GetUnitResponse>;

  createUnit: (
    req: Request<never, never, CreateUnitDto>,
    res: Response,
  ) => ControllerResponseMessage<CreateUnitResponse>;

  updateUnit: (
    req: Request<UnitParamsReq, never, UpdateUnitDto>,
    res: Response,
  ) => ControllerResponseMessage<UpdateUnitResponse>;

  deleteUnit: (
    req: Request<UnitParamsReq>,
    res: Response,
  ) => ControllerResponseMessage<DeleteUnitResponse>;
}
