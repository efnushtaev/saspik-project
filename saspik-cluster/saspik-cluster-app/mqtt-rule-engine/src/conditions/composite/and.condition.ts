import { Condition } from '../condition.interface';
import { MessageContext } from '../../context';

/**
 * Логическое И (AND) для нескольких условий.
 */
export class AndCondition implements Condition {
  /**
   * Создаёт условие.
   * @param conditions - массив условий, которые должны все быть истинными.
   */
  constructor(private conditions: Condition[]) {}

  evaluate(ctx: MessageContext): boolean {
    if (this.conditions.length === 0) {
      return true;
    }
    for (const cond of this.conditions) {
      if (!cond.evaluate(ctx)) {
        return false;
      }
    }
    return true;
  }
}