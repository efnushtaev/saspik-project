import { Condition } from './condition.interface';
import { MessageContext } from '../context';

/**
 * Условие, проверяющее точное совпадение payload (как строки).
 */
export class PayloadEqualsCondition implements Condition {
  /**
   * Создаёт условие.
   * @param expected - ожидаемая строка payload.
   */
  constructor(private expected: string) {}

  evaluate(ctx: MessageContext): boolean {
    return ctx.payloadString === this.expected;
  }
}