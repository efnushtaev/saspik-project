import { Condition } from './condition.interface';
import { MessageContext } from '../context';

/**
 * Условие, проверяющее топик по регулярному выражению.
 */
export class TopicRegexCondition implements Condition {
  private regex: RegExp;

  /**
   * Создаёт условие.
   * @param pattern - строка регулярного выражения (например, "^sensor/.+$").
   */
  constructor(pattern: string) {
    this.regex = new RegExp(pattern);
  }

  evaluate(ctx: MessageContext): boolean {
    return this.regex.test(ctx.topic);
  }
}