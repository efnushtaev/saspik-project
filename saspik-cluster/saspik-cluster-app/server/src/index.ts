import { Container, ContainerModule, interfaces } from "inversify";

import { type IExeptionFilter } from "./errors/exeption.filter.interface";
import { TYPES } from "./types";
import { App } from "./app";
import { type IConfigService } from "./config/config.service.interface";
import { type ILogger } from "./logger/logger.interface";
import { LoggerService } from "./logger/loggerService";
import { ObjectsController } from "./controllers/objects.controller";
import { ExeptionFilter } from "./errors/exeption.filter";
import { ConfigService } from "./config/config.service";
import { type Mongo, MongoService } from "./services/data-store";
import {
  UnitsRepository,
  ObjectsRepository,
  RulesRepository,
  SeedService,
} from "./services/data-store";
import { type IMqttService, MqttService } from "./services/mqtt";
import { LocalMqttService } from "./services/mqtt/localMqtt.service";
import {
  type IClimateControlService,
  ClimateControlService,
} from "./services/climate-control";
import { RelayControllerOptions } from "./services/climate-control/types";
import { type IUnitsService, UnitsService } from "./services/units";
import { type IObjectsService, ObjectsService } from "./services/objects";
import { type IRulesService, RulesService } from "./services/rules";
import { type IUnitsController } from "./controllers/units.controller.interface";
import { UnitsController } from "./controllers/units.controller";
import { type IObjectsController } from "./controllers/objects.controller.interface";
import { RulesController } from "./controllers/rules.controller";
import { MqttController } from "./controllers/mqtt.controller";
import {
  type IStateStoreService,
  InfluxDbStateStoreService,
} from "./services/state-store";

const appBindings = new ContainerModule((bind: interfaces.Bind) => {
  bind<IObjectsController>(TYPES.ObjectsController)
    .to(ObjectsController)
    .inSingletonScope();
  bind<IUnitsController>(TYPES.UnitsController)
    .to(UnitsController)
    .inSingletonScope();
  bind<RulesController>(TYPES.RulesController)
    .to(RulesController)
    .inSingletonScope();
  bind<MqttController>(TYPES.MqttController)
    .to(MqttController)
    .inSingletonScope();
  bind<Mongo>(TYPES.Mongo).to(MongoService).inSingletonScope();
  bind<UnitsRepository>(TYPES.UnitsRepository)
    .to(UnitsRepository)
    .inSingletonScope();
  bind<ObjectsRepository>(TYPES.ObjectsRepository)
    .to(ObjectsRepository)
    .inSingletonScope();
  bind<RulesRepository>(TYPES.RulesRepository)
    .to(RulesRepository)
    .inSingletonScope();
  bind<SeedService>(TYPES.SeedService).to(SeedService).inSingletonScope();
  bind<IMqttService>(TYPES.MqttService).to(MqttService).inSingletonScope();
  bind<LocalMqttService>(TYPES.LocalMqttService)
    .to(LocalMqttService)
    .inSingletonScope();
  bind<IMqttService>(TYPES.RightechProxyMqttService)
    .to(LocalMqttService)
    .inSingletonScope();
  bind<RelayControllerOptions>(TYPES.RelayControllerOptions).toConstantValue(
    {},
  );
  bind<IUnitsService>(TYPES.UnitsService).to(UnitsService).inSingletonScope();
  bind<IObjectsService>(TYPES.ObjectsService)
    .to(ObjectsService)
    .inSingletonScope();
  bind<IRulesService>(TYPES.RulesService).to(RulesService).inSingletonScope();
  bind<IStateStoreService>(TYPES.StateStoreService)
    .to(InfluxDbStateStoreService)
    .inSingletonScope();
  bind<IClimateControlService>(TYPES.ClimateControlService)
    .to(ClimateControlService)
    .inSingletonScope();
  bind<ILogger>(TYPES.Logger).to(LoggerService).inSingletonScope();
  bind<IExeptionFilter>(TYPES.ExeptionFilter)
    .to(ExeptionFilter)
    .inSingletonScope();
  bind<IConfigService>(TYPES.ConfigService)
    .to(ConfigService)
    .inSingletonScope();
  bind<App>(TYPES.Application).to(App);
});

async function bootstrap() {
  const appContainer = new Container();
  appContainer.load(appBindings);

  // Подключаемся к MongoDB до создания приложения
  const mongo = appContainer.get<Mongo>(TYPES.Mongo);
  await mongo.connect();

  const app = appContainer.get<App>(TYPES.Application);
  await app.init();
}

bootstrap();
