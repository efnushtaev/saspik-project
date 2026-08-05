import { Request, Response } from "express";

import { ControllerResponseMessage } from "../common/controller.types";
import { ObjectsType, ObjectsDto } from "../dto/objects.dto";

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

export type ListBodyReq = {
  unitId?: string;
};

export type GetByIdsBodyReq = {
  id: string[];
  type?: ObjectsType;
  unitId?: string;
};

export type CommandBodyReq = {
  value: string;
  unitId?: string;
};

export type CommandParamsReq = {
  deviceId: string;
};

export type LastSensorsDataBodyReq = {
  id: string[];
};

export type CreateObjectBodyReq = Omit<ObjectsDto, "topic"> & {
  unitId: string;
};

export type UpdateObjectParamsReq = {
  id: string;
};

export type UpdateObjectBodyReq = Omit<ObjectsDto, "topic" | "id"> & {
  unitId: string;
};

export type DeleteObjectParamsReq = {
  id: string;
};

export type DeleteObjectBodyReq = {
  unitId: string;
};

export type GetByIdsResponse = ListResponse;
export type CommandResponse = { success: boolean };
export type LastSensorsDataResponse = Record<string, unknown>;
export type CreateObjectResponse = { object: ObjectsDto };
export type UpdateObjectResponse = { object: ObjectsDto };
export type DeleteObjectResponse = { success: boolean };

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

  createObject: (
    req: Request<never, never, CreateObjectBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<CreateObjectResponse>;

  updateObject: (
    req: Request<UpdateObjectParamsReq, never, UpdateObjectBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<UpdateObjectResponse>;

  deleteObject: (
    req: Request<DeleteObjectParamsReq, never, DeleteObjectBodyReq>,
    res: Response,
  ) => ControllerResponseMessage<DeleteObjectResponse>;
}
