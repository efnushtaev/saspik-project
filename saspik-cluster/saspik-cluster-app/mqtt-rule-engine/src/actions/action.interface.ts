import { MessageContext } from '../context';
import { IMqttAdapter } from '../mqtt';

/**
 * Интерфейс действия.
 * Действие выполняет какую-либо операцию на основе контекста сообщения.
 */
export interface Action {
  /**
   * Выполняет действие.
   * @param ctx - контекст сообщения.
   * @param publisher - адаптер MQTT для публикации (если нужно).
   * @returns Promise, разрешающийся после завершения действия.
   */
  execute(ctx: MessageContext, publisher: IMqttAdapter): Promise<void>;
}