import { TEMPORARY_ANY } from "../../types";

export interface IMqttService {
  /**
   * Публикация сообщения в топик MQTT
   * @param topic - топик для публикации
   * @param message - сообщение (строка или Buffer)
   * @param options - дополнительные опции публикации (например, qos, retain)
   * @returns Promise, разрешающийся после успешной публикации
   */
  publish: (
    topic: string,
    message: string | Buffer,
    options?: TEMPORARY_ANY,
  ) => Promise<void>;

  /**
   * Подписка на топик MQTT
   * @param topic - топик для подписки (может содержать wildcards)
   * @param callback - функция обратного вызова, вызываемая при получении сообщения
   * @returns Promise, разрешающийся после успешной подписки
   */
  subscribe: (
    topic: string,
    callback: (topic: string, message: Buffer) => void,
    options?: TEMPORARY_ANY,
  ) => Promise<void>;

  /**
   * Отписка от топика
   * @param topic - топик для отписки
   * @returns Promise, разрешающийся после успешной отписки
   */
  unsubscribe: (topic: string) => Promise<void>;

  /**
   * Подключение к MQTT брокеру (если требуется явное подключение)
   * @returns Promise, разрешающийся после успешного подключения
   */
  connect?: () => Promise<void>;

  /**
   * Отключение от MQTT брокера
   * @returns Promise, разрешающийся после успешного отключения
   */
  disconnect?: () => Promise<void>;
}
