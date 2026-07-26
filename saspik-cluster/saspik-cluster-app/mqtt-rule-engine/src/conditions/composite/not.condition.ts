import { Condition } from '../condition.interface';
import { MessageContext } from '../../context';

/**
 * Логическое НЕ (NOT) для одного условия.
 */
export class NotCondition implements Condition {
  /**
   * Создаёт условие.
   * @param condition - условие, которое должно быть ложным.
   */
  constructor(private condition: Condition) {}

  evaluate(ctx: MessageContext): boolean {
    return !this.condition.evaluate(ctx);
  }
}