import { Request, Response } from "express";

import { ControllerResponseMessage } from "../common/controller.types";
import { ObjectsType } from "../dto/objects.dto";

export interface ObjectsListItem {
  id: string;
  name: string;
  type: "sensor" | "device";
  spec: {
    key: string;
    value: number | string | boolean | null;
    spec: {
      model: string;
      unit?: string;
      minorPart?: number;
    };
  }[];
  description?: string;
  topic: string;
}

export interface ListResponse {
  objects: ObjectsListItem[];
}

export type ListParamsReq = {
  type: ObjectsType;
};

export type ListBodyReq = Record<string, never>;

export type GetByIdsBodyReq = {
  id: string[];
  type?: ObjectsType;
};

export type CommandBodyReq = {
  value: string;
};

export type CommandParamsReq = {
  deviceId: string;
};

export type LastSensorsDataBodyReq = {
  id: string[];
};

export type GetByIdsResponse = ListResponse;
export type CommandResponse = { success: boolean };
export type LastSensorsDataResponse = Record<string, unknown>;

export interface IObjectsController {
  getObjectsLists: (
    req: Request<ListParamsReq, never, ListBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<ListResponse>;

  getByIds: (
    req: Request<never, never, GetByIdsBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<GetByIdsResponse>;

  callCommand: (
    req: Request<CommandParamsReq, never, CommandBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<CommandResponse>;

  getLastSensorsData: (
    req: Request<never, never, LastSensorsDataBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<LastSensorsDataResponse>;
}
