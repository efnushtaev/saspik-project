import { Server } from "http";
import express, { Express } from "express";
import { inject, injectable } from "inversify";
import bodyParser from "body-parser";

import { type ILogger } from "./logger/logger.interface";
import { TYPES } from "./types";
import { type IConfigService } from "./config/config.service.interface";
import "reflect-metadata";
import { type IExeptionFilter } from "./errors/exeption.filter.interface";
import { ObjectsController } from "./controllers/objects.controller";
import { API_V1_URL_PREFIX, ControllersDomens } from "./const";
import { UnitsController } from "./controllers/units.controller";
import { MqttController } from "./controllers/mqtt.controller";
import { type IClimateControlService } from "./services/climate-control/climateControl.interface";
import { InfluxDbStateStoreService } from "./services/state-store";
import { IObjectsService } from "./services/objects";
import { ObjectsDto } from "./dto/objects.dto";

@injectable()
export class App {
  app: Express;
  port: number;
  server: Server;

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ObjectsController)
    private objectsController: ObjectsController,
    @inject(TYPES.UnitsController)
    private unitsController: UnitsController,
    @inject(TYPES.MqttController)
    private mqttController: MqttController,
    @inject(TYPES.ExeptionFilter) private exeptionFilter: IExeptionFilter,
    @inject(TYPES.ConfigService) private configService: IConfigService,
    @inject(TYPES.ClimateControlService)
    private climateControlService: IClimateControlService,
    @inject(TYPES.StateStoreService)
    private stateStore: InfluxDbStateStoreService,
    @inject(TYPES.ObjectsService)
    private objectsService: IObjectsService,
  ) {
    this.app = express();
    this.port = parseInt(process.env.PORT || "3001", 10);
  }

  private useMiddleware(): void {
    this.app.use(bodyParser.json());
  }

  private useRoutes(): void {
    // Health check endpoint
    this.app.get("/health", (req, res) => {
      res.status(200).send("OK");
    });

    this.app.get("/api/getTimestamp", (req, res) => {
      res.json({ timestamp: new Date().toISOString() });
    });

    this.app.use(
      `${API_V1_URL_PREFIX}${ControllersDomens.OBJECTS}`,
      this.objectsController.router,
    );
    this.app.use(
      `${API_V1_URL_PREFIX}${ControllersDomens.UNITS}`,
      this.unitsController.router,
    );
  }

  private useExeptionFilters(): void {
    this.app.use(this.exeptionFilter.catch.bind(this.exeptionFilter));
  }

  protected async afterStart(): Promise<void> {
    this.logger.log("[App] afterStart hook executed");

    const unitId = this.configService.get("CLIMATE_CONTROL_UNIT_ID");
    if (unitId) {
      this.logger.log(`[App] Starting climate control for unit ${unitId}`);
      // await this.climateControlService.executeControll(unitId);
    } else {
      this.logger.log(
        "[App] CLIMATE_CONTROL_UNIT_ID not set, skipping climate control",
      );
    }

    const objects = await this.objectsService.getObjects();
    await this.stateStore.init(objects as ObjectsDto[]);
  }

  public async init(): Promise<void> {
    this.useMiddleware();
    this.useRoutes();
    this.useExeptionFilters();

    this.server = this.app.listen(this.port);
    this.logger.log(`Сервер запущен на http://localhost:${this.port}`);

    await this.afterStart();
  }
}
