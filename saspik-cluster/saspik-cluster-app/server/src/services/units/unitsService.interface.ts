import { UnitDto, CreateUnitDto, UpdateUnitDto } from "../../dto/units.dto";
import { TEMPORARY_ANY } from "../../types";

export interface IUnitsService {
  /**
   * Получить список всех юнитов
   * @returns массив сущностей Unit
   */
  getUnits(): Promise<UnitDto[]>;

  /**
   * Получить юнит по ID
   * @param id - идентификатор юнита
   * @returns сущность Unit или null, если юнит не найден
   */
  getUnitById(id: string): Promise<UnitDto | null>;

  /**
   * Создать юнит
   * @param dto - данные юнита (id, name, description?)
   * @returns созданная сущность Unit
   */
  createUnit(dto: CreateUnitDto): Promise<UnitDto>;

  /**
   * Обновить юнит (id не изменяем)
   * @param id - идентификатор юнита
   * @param dto - данные для обновления (name, description?)
   * @returns обновлённая сущность Unit или null, если юнит не найден
   */
  updateUnit(id: string, dto: UpdateUnitDto): Promise<UnitDto | null>;

  /**
   * Удалить юнит вместе с его объектами (каскадно). Правила не затрагиваются.
   * @param id - идентификатор юнита
   * @returns true, если юнит был удалён
   */
  deleteUnit(id: string): Promise<boolean>;

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
