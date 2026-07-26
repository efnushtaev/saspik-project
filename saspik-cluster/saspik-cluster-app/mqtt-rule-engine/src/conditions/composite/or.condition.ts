import { Condition } from '../condition.interface';
import { MessageContext } from '../../context';

/**
 * Логическое ИЛИ (OR) для нескольких условий.
 */
export class OrCondition implements Condition {
  /**
   * Создаёт условие.
   * @param conditions - массив условий, из которых хотя бы одно должно быть истинным.
   */
  constructor(private conditions: Condition[]) {}

  evaluate(ctx: MessageContext): boolean {
    if (this.conditions.length === 0) {
      return true;
    }
    for (const cond of this.conditions) {
      if (cond.evaluate(ctx)) {
        return true;
      }
    }
    return false;
  }
}