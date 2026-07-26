import { UnitDto } from "../../dto/units.dto";
import { TEMPORARY_ANY } from "../../types";

export interface IUnitsService {
  /**
   * Получить список всех юнитов
   * @returns массив сущностей Unit
   */
  getUnits(): Promise<UnitDto[]>;

  /**
   * Отправить команду юниту через MQTT
   * @param unitId - идентификатор юнита
   * @param command - команда (например, "on", "off")
   * @param payload - дополнительные данные команды (опционально)
   * @returns Promise, разрешающийся после успешной публикации сообщения
   */
  callCommand(
    unitId: string,
    command: string,
    payload?: TEMPORARY_ANY,
  ): Promise<void>;
}
