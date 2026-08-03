export const API_V1_URL_PREFIX = "/api/v1";

export enum ControllersDomens {
  OBJECTS = "/objects",
  UNITS = "/units",
  RULES = "/rules",
  MQTT = "/mqtt",
}

export enum UnitsControllersRoutesURL {
  UNITS_LIST = "/list",
}

export enum RulesControllersRoutesURL {
  RULES_LIST = "/",
  RULES_CREATE = "/",
  RULES_UPDATE = "/:id",
  RULES_DELETE = "/:id",
}

export enum ObjectsControllersRoutesURL {
  OBJECTS_LIST = "/list/:type",
  OBJECTS_GET_BY_IDS = "/getByIds",
  OBJECTS_COMMAND = "/command/:deviceId",
  OBJECTS_LAST_SENSORS_DATA = "/getLastSensorsData",
}

export const MQTT_BROCKER_API_URL = "https://dev.rightech.io/api/v1";

export enum RequestMethod {
  GET = "get",
  POST = "post",
  PUT = "put",
  DELETE = "delete",
  PATCH = "patch",
}

export const PERIOD_1H = 3600;
