import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IMqttService } from "../mqtt/mqtt.interface";
import { TYPES } from "../../types";
import {
  type Commands,
  type RelayControllerOptions,
  type SensorData,
} from "./types";
import { IClimateControlService } from ".";
import { SensorObjectsType } from "../../dto/objects.dto";

@injectable()
export class ClimateControlService implements IClimateControlService {
  private T_SET: number;
  private T_HYST: number;
  private RH_SET: number;
  private RH_HYST: number;
  private T_MAX: number;
  private T_MIN: number;

  private NIGHT_T_SET: number;
  private NIGHT_T_HYST: number;
  private NIGHT_RH_SET: number;
  private NIGHT_RH_HYST: number;
  private NIGHT_T_MAX: number;
  private NIGHT_T_MIN: number;
  private NIGHT_START_HOUR: number;
  private NIGHT_END_HOUR: number;
  private MOSCOW_OFFSET_HOUR: number;

  private fanStateMap = new Map<string, boolean>();
  private humStateMap = new Map<string, boolean>();
  private lightStateMap = new Map<string, boolean>();
  private waterStateMap = new Map<string, boolean>();
  private intervals = new Map<string, NodeJS.Timeout>();
  // Кэш последних данных с датчиков по unitId
  private sensorDataCache = new Map<string, SensorData>();
  // Флаг подписки на топик sensors для каждого юнита
  private subscribedTopics = new Set<string>();

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
    @inject(TYPES.MqttService) private mqttService: IMqttService,
    @inject(TYPES.RelayControllerOptions) options: RelayControllerOptions = {},
  ) {
    this.T_SET = options.T_SET ?? this.getConfigNumber("CLIMATE_T_SET", 25);
    this.T_HYST = options.T_HYST ?? this.getConfigNumber("CLIMATE_T_HYST", 1);
    this.RH_SET = options.RH_SET ?? this.getConfigNumber("CLIMATE_RH_SET", 70);
    this.RH_HYST =
      options.RH_HYST ?? this.getConfigNumber("CLIMATE_RH_HYST", 10);
    this.T_MAX = options.T_MAX ?? this.getConfigNumber("CLIMATE_T_MAX", 27);
    this.T_MIN = options.T_MIN ?? this.getConfigNumber("CLIMATE_T_MIN", 23);

    this.NIGHT_T_SET = this.getConfigNumber("CLIMATE_NIGHT_T_SET", 22);
    this.NIGHT_T_HYST = this.getConfigNumber(
      "CLIMATE_NIGHT_T_HYST",
      this.T_HYST,
    );
    this.NIGHT_RH_SET = this.getConfigNumber("CLIMATE_NIGHT_RH_SET", 60);
    this.NIGHT_RH_HYST = this.getConfigNumber(
      "CLIMATE_NIGHT_RH_HYST",
      this.RH_HYST,
    );
    this.NIGHT_T_MAX = this.getConfigNumber("CLIMATE_NIGHT_T_MAX", 24);
    this.NIGHT_T_MIN = this.getConfigNumber("CLIMATE_NIGHT_T_MIN", 20);
    this.NIGHT_START_HOUR = this.getConfigNumber(
      "CLIMATE_NIGHT_START_HOUR",
      22,
    );
    this.NIGHT_END_HOUR = this.getConfigNumber("CLIMATE_NIGHT_END_HOUR", 8);
    this.MOSCOW_OFFSET_HOUR = this.getConfigNumber("MOSCOW_OFFSET_HOUR", 3);

    // State maps are initialized empty

    this.logger.log("[ClimateControlService] initialized with settings:", {
      T_SET: this.T_SET,
      T_HYST: this.T_HYST,
      RH_SET: this.RH_SET,
      RH_HYST: this.RH_HYST,
      T_MAX: this.T_MAX,
      T_MIN: this.T_MIN,
      NIGHT_T_SET: this.NIGHT_T_SET,
      NIGHT_T_HYST: this.NIGHT_T_HYST,
      NIGHT_RH_SET: this.NIGHT_RH_SET,
      NIGHT_RH_HYST: this.NIGHT_RH_HYST,
      NIGHT_T_MAX: this.NIGHT_T_MAX,
      NIGHT_T_MIN: this.NIGHT_T_MIN,
      NIGHT_START_HOUR: this.NIGHT_START_HOUR,
      NIGHT_END_HOUR: this.NIGHT_END_HOUR,
      MOSCOW_OFFSET_HOUR: this.MOSCOW_OFFSET_HOUR,
    });
  }

  private getConfigNumber(key: string, defaultValue: number): number {
    const value = this.config.get(key);
    if (value === undefined) {
      return defaultValue;
    }
    const parsed = Number(value);
    return isNaN(parsed) ? defaultValue : parsed;
  }

  private isNight(): boolean {
    const now = new Date();

    const hour = now.getHours() + this.MOSCOW_OFFSET_HOUR;
    const { NIGHT_START_HOUR, NIGHT_END_HOUR } = this;
    // Handle overnight range (e.g., 22 to 6)
    if (NIGHT_START_HOUR <= NIGHT_END_HOUR) {
      // Range does not cross midnight
      return hour >= NIGHT_START_HOUR && hour < NIGHT_END_HOUR;
    } else {
      // Range crosses midnight (e.g., 22 to 6)
      return hour >= NIGHT_START_HOUR || hour < NIGHT_END_HOUR;
    }
  }

  private parseSensorDataFromMessage(message: Buffer): SensorData {
    try {
      const data = JSON.parse(message.toString());
      const objectsList: Array<{
        sensorType?: SensorObjectsType;
        value?: number | string | boolean;
      }> = data.objectsList || [];
      const tempObj = objectsList.find(
        (obj) =>
          obj.sensorType === SensorObjectsType.TEMPERATURE &&
          typeof obj.value === "number",
      );
      const humidityObj = objectsList.find(
        (obj) =>
          obj.sensorType === SensorObjectsType.HUMIDITY &&
          typeof obj.value === "number",
      );
      const floatSensorObj = objectsList.find(
        (obj) =>
          obj.sensorType === SensorObjectsType.FLOAT_SENSOR &&
          typeof obj.value === "number",
      );
      this.logger.log(
        "[parseSensorDataFromMessage] data.objectsList:",
        data.objectsList,
      );
      if (!tempObj || !humidityObj || !floatSensorObj) {
        throw new Error(
          "Missing temperature or humidity or Float in sensor data",
        );
      }
      const temperature = tempObj.value as number;
      const humidity = humidityObj.value as number;
      const floatSensor = floatSensorObj.value as number;
      return { temperature, humidity, floatSensor };
    } catch (error) {
      this.logger.error(
        "[ClimateControlService] failed to parse sensor data from message:",
        error,
      );
      throw error;
    }
  }

  /**
   * Установить постоянную подписку на топик sensors для указанного юнита, если её ещё нет.
   * Данные будут сохраняться в кэш sensorDataCache.
   */
  private async ensureSubscription(unitId: string): Promise<void> {
    const topic = `units/${unitId}/sensors`;
    if (this.subscribedTopics.has(topic)) {
      return;
    }
    this.logger.log(
      `[ClimateControlService] establishing permanent subscription to ${topic}`,
    );
    const callback = (receivedTopic: string, message: Buffer) => {
      this.logger.log(
        `[ClimateControlService] received sensor data via permanent subscription for ${receivedTopic}`,
      );
      try {
        const sensorData = this.parseSensorDataFromMessage(message);
        this.sensorDataCache.set(unitId, sensorData);
        this.logger.log(
          `[ClimateControlService] updated sensor cache for unit ${unitId}:`,
          sensorData,
        );
      } catch (error) {
        this.logger.error(
          `[ClimateControlService] error parsing sensor data in permanent subscription:`,
          error,
        );
      }
    };
    await this.mqttService.subscribe(topic, callback, { id: unitId });
    this.subscribedTopics.add(topic);
    this.logger.log(
      `[ClimateControlService] permanent subscription established for ${topic}`,
    );
  }

  /**
   * Обновить состояние на основе новых данных с датчиков
   * @param unitId - идентификатор юнита
   * @param sensorData - { temperature, humidity }
   * @returns команды для исполнительных устройств (fan, humidifier, light)
   */
  update(unitId: string, sensorData: SensorData): Commands {
    const { temperature, humidity, floatSensor } = sensorData;
    const commands: Commands = {};

    // Управление освещением (включено днем, выключено ночью)
    const isNight = this.isNight();
    const desiredLightState = !isNight;
    const currentLightState = this.lightStateMap.get(unitId) ?? false;
    if (currentLightState !== desiredLightState) {
      commands.light = desiredLightState;
    }

    // Управление водой: если floatSensor === 0, включить воду
    const desiredWaterState = floatSensor === 0;
    const currentWaterState = this.waterStateMap.get(unitId) ?? false;
    if (currentWaterState !== desiredWaterState) {
      commands.water = desiredWaterState;
    }

    const T_SET = isNight ? this.NIGHT_T_SET : this.T_SET;
    const T_HYST = isNight ? this.NIGHT_T_HYST : this.T_HYST;
    const RH_SET = isNight ? this.NIGHT_RH_SET : this.RH_SET;
    const RH_HYST = isNight ? this.NIGHT_RH_HYST : this.RH_HYST;
    const T_MAX = isNight ? this.NIGHT_T_MAX : this.T_MAX;
    const T_MIN = isNight ? this.NIGHT_T_MIN : this.T_MIN;

    const currentFanState = this.fanStateMap.get(unitId) ?? false;
    const currentHumState = this.humStateMap.get(unitId) ?? false;

    // Аварийные приоритеты
    if (temperature > T_MAX) {
      commands.fan = true; // включить вытяжку
      commands.humidifier = false; // выключить увлажнитель
      this.fanStateMap.set(unitId, true);
      this.humStateMap.set(unitId, false);
      if (commands.light !== undefined) {
        this.lightStateMap.set(unitId, commands.light);
      }
      return commands;
    }
    if (temperature < T_MIN) {
      commands.fan = false; // выключить вытяжку
      commands.humidifier = false;
      this.fanStateMap.set(unitId, false);
      this.humStateMap.set(unitId, false);
      if (commands.light !== undefined) {
        this.lightStateMap.set(unitId, commands.light);
      }
      return commands;
    }

    // Температурный контур
    if (temperature > T_SET + T_HYST / 2) {
      if (!currentFanState) commands.fan = true;
    } else if (temperature < T_SET - T_HYST / 2) {
      if (currentFanState) commands.fan = false;
    }

    // Влажностный контур
    if (humidity < RH_SET - RH_HYST / 2) {
      if (!currentHumState) commands.humidifier = true;
    } else if (humidity > RH_SET + RH_HYST / 2) {
      if (currentHumState) commands.humidifier = false;
    }

    // Обновить внутреннее состояние
    if (commands.fan !== undefined) {
      this.fanStateMap.set(unitId, commands.fan);
    }
    if (commands.humidifier !== undefined) {
      this.humStateMap.set(unitId, commands.humidifier);
    }
    if (commands.light !== undefined) {
      this.lightStateMap.set(unitId, commands.light);
    }
    if (commands.water !== undefined) {
      this.waterStateMap.set(unitId, commands.water);
    }

    return commands;
  }

  /**
   * Получить данные с датчиков климата используя подписку MQTT
   * @param unitId - идентификатор юнита
   * @returns данные с датчиков температуры и влажности
   */
  async getSensorsData(unitId: string): Promise<SensorData> {
    this.logger.log(
      `[ClimateControlService] fetching sensor data for unit ${unitId}`,
    );
    // Если в кэше уже есть данные, возвращаем их сразу
    const cached = this.sensorDataCache.get(unitId);
    if (cached) {
      this.logger.log(
        `[ClimateControlService] using cached sensor data for unit ${unitId}:`,
        cached,
      );
      return cached;
    }
    // Иначе ждём появления данных в кэше (постоянная подписка уже должна быть установлена)
    this.logger.log(
      `[ClimateControlService] waiting for sensor data to appear in cache for unit ${unitId}`,
    );
    return new Promise<SensorData>((resolve, reject) => {
      // Таймаут на случай, если данные не поступят в течение 2 минут.
      const timeout = setTimeout(() => {
        this.logger.warn(
          `[ClimateControlService] timeout waiting for sensor data from unit ${unitId}`,
        );
        reject(
          new Error(`Timeout waiting for sensor data from unit ${unitId}`),
        );
      }, 120000); // 2 minutes

      // Одноразовый наблюдатель за кэшем
      const checkInterval = setInterval(() => {
        const data = this.sensorDataCache.get(unitId);
        if (data) {
          clearInterval(checkInterval);
          clearTimeout(timeout);
          this.logger.log(
            `[ClimateControlService] received sensor data via cache for unit ${unitId}:`,
            data,
          );
          resolve(data);
        }
      }, 1000); // проверяем каждую секунду
    });
  }

  /**
   * Запустить фоновый контроль климата для указанного юнита.
   * Метод запускает периодическое обновление (update) на основе данных с датчиков.
   * @param unitId - идентификатор юнита
   */
  async executeControll(unitId: string): Promise<void> {
    this.logger.log(
      `[ClimateControlService] starting climate control for unit ${unitId}`,
    );
    // Установить постоянную подписку на данные датчиков
    try {
      await this.ensureSubscription(unitId);
    } catch (error) {
      this.logger.error(
        `[ClimateControlService] failed to establish permanent subscription for unit ${unitId}:`,
        error,
      );
      // Продолжаем, возможно, данные появятся позже
    }
    // Если уже есть таймаут для этого юнита, очистить его
    const existingTimeout = this.intervals.get(unitId);
    if (existingTimeout) {
      clearTimeout(existingTimeout);
      this.intervals.delete(unitId);
    }

    // Интервал обновления (по умолчанию 30 секунд)
    const intervalMs = this.getConfigNumber(
      "CLIMATE_CONTROL_INTERVAL_MS",
      30000,
    );

    const runLoop = async () => {
      this.logger.log(
        `[ClimateControlService] control loop iteration start for unit ${unitId}`,
      );
      try {
        const sensorData = await this.getSensorsData(unitId);
        const commands = this.update(unitId, sensorData);
        this.logger.log(
          `[ClimateControlService] computed commands for unit ${unitId}:`,
          commands,
        );
        // Отправка команд через MQTT
        if (commands.fan !== undefined) {
          await this.mqttService.publish(
            `units/${unitId}/commands/a_relay3`,
            JSON.stringify(commands.fan ? "1" : "0"),
          );
        }
        if (commands.humidifier !== undefined) {
          await this.mqttService.publish(
            `units/${unitId}/commands/a_relay2`,
            JSON.stringify(commands.humidifier ? "1" : "0"),
          );
        }
        if (commands.light !== undefined) {
          await this.mqttService.publish(
            `units/${unitId}/commands/a_relay1`,
            JSON.stringify(commands.light ? "1" : "0"),
          );
        }
        if (commands.water !== undefined) {
          await this.mqttService.publish(
            `units/${unitId}/commands/a_relay4`,
            JSON.stringify(commands.water ? "1" : "0"),
          );

          setTimeout(() => {
            this.mqttService.publish(
              `units/${unitId}/commands/a_relay4`,
              JSON.stringify("0"),
            );
          }, 30000);
        }
        this.logger.log(
          `[ClimateControlService] control loop iteration completed for unit ${unitId}`,
        );
      } catch (error) {
        this.logger.error(
          `[ClimateControlService] error in control loop for unit ${unitId}:`,
          error,
        );
      } finally {
        // Запланировать следующую итерацию после intervalMs
        const timeout = setTimeout(runLoop, intervalMs);
        this.intervals.set(unitId, timeout);
      }
    };

    // Запустить первую итерацию немедленно
    const timeout = setTimeout(runLoop, 0);
    this.intervals.set(unitId, timeout);
  }
}
