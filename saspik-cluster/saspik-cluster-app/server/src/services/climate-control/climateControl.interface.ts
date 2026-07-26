import { type SensorData, type Commands } from "./types";

export interface IClimateControlService {
  /**
   * Обновить состояние на основе новых данных с датчиков
   * @param unitId - идентификатор юнита
   * @param sensorData - { temperature, humidity }
   * @returns команды для исполнительных устройств (fan, humidifier, light)
   */
  update(unitId: string, sensorData: SensorData): Commands;

  /**
   * Получить данные с датчиков климата используя unitService
   * @param unitId - идентификатор юнита
   * @returns данные с датчиков температуры и влажности
   */
  getSensorsData(unitId: string): Promise<SensorData>;

  /**
   * Запустить фоновый контроль климата для указанного юнита.
   * Метод запускает периодическое обновление (update) на основе данных с датчиков.
   * @param unitId - идентификатор юнита
   */
  executeControll(unitId: string): Promise<void>;
}
