import { Response, Request } from "express";
import { inject, injectable } from "inversify";

import "reflect-metadata";
import {
  IObjectsController,
  ListParamsReq,
  ListBodyReq,
  ListResponse,
  GetByIdsBodyReq,
  CommandBodyReq,
  CommandParamsReq,
  CommandResponse,
  LastSensorsDataBodyReq,
  LastSensorsDataResponse,
} from "./objects.controller.interface";
import { BaseController } from "../common/baseController";
import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { IObjectsService } from "../services/objects";
import { ObjectsControllersRoutesURL, RequestMethod } from "../const";

function formatValue(
  value: number | string | boolean | null,
  minorPart?: number,
): number | string | boolean | null {
  if (typeof value === "number" && minorPart !== undefined) {
    return value.toFixed(minorPart);
  }
  return value;
}

@injectable()
export class ObjectsController
  extends BaseController
  implements IObjectsController
{
  constructor(
    @inject(TYPES.Logger) private loggerService: ILogger,
    @inject(TYPES.ObjectsService)
    private objectsService: IObjectsService,
  ) {
    super(loggerService);
    this.bindRoutes([
      {
        path: ObjectsControllersRoutesURL.OBJECTS_LIST,
        method: RequestMethod.POST,
        func: this.getObjectsLists,
      },
      {
        path: ObjectsControllersRoutesURL.OBJECTS_GET_BY_IDS,
        method: RequestMethod.POST,
        func: this.getByIds,
      },
      {
        path: ObjectsControllersRoutesURL.OBJECTS_COMMAND,
        method: RequestMethod.POST,
        func: this.callCommand,
      },
      {
        path: ObjectsControllersRoutesURL.OBJECTS_LAST_SENSORS_DATA,
        method: RequestMethod.POST,
        func: this.getLastSensorsData,
      },
    ]);
  }

  async getObjectsLists(
    { params }: Request<ListParamsReq, never, ListBodyReq>,
    res: Response,
  ) {
    const typeFilter = params.type;
    const objects = await this.objectsService.getObjects(typeFilter);
    const enriched = await Promise.all(
      objects.map(async (object) => {
        const spec = await Promise.all(
          object.spec.map(async (s) => {
            const raw = await this.objectsService.getObjectState(object.id, s.key);
            return { key: s.key, value: formatValue(raw, s.minorPart), spec: s };
          }),
        );
        return { ...object, spec };
      }),
    );
    return this.ok<ListResponse>(res, { objects: enriched });
  }

  async getByIds(
    { body }: Request<never, never, GetByIdsBodyReq>,
    res: Response,
  ) {
    const objects = await this.objectsService.getByIds(body.id, body.type);
    const enriched = await Promise.all(
      objects.map(async (object) => {
        const spec = await Promise.all(
          object.spec.map(async (s) => {
            const raw = await this.objectsService.getObjectState(object.id, s.key);
            return { key: s.key, value: formatValue(raw, s.minorPart), spec: s };
          }),
        );
        return { ...object, spec };
      }),
    );
    return this.ok<ListResponse>(res, { objects: enriched });
  }

  async callCommand(
    { params, body }: Request<CommandParamsReq, never, CommandBodyReq>,
    res: Response,
  ) {
    try {
      await this.objectsService.callCommand(params.deviceId, body.value);
      return this.ok<CommandResponse>(res, { success: true });
    } catch (error) {
      return this.send(res, 500, {
        success: false,
        error: error instanceof Error ? error.message : "Unknown error",
      });
    }
  }

  async getLastSensorsData(
    { body }: Request<never, never, LastSensorsDataBodyReq>,
    res: Response,
  ) {
    const data = await this.objectsService.getLastSensorsData(body.id);
    return this.ok<LastSensorsDataResponse>(res, data);
  }
}
