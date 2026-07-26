import { MessageContext } from '../context';

/**
 * Интерфейс условия.
 * Условие оценивает контекст сообщения и возвращает true/false.
 */
export interface Condition {
  /**
   * Выполняет проверку условия на основе контекста сообщения.
   * @param ctx - контекст сообщения.
   * @returns true, если условие выполнено; иначе false.
   */
  evaluate(ctx: MessageContext): boolean;
}