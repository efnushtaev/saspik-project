/**
 * Интерфейс источника правил для Rule Engine.
 */

import { Rule } from '../config';

export interface RulesProvider {
  /** Человекочитаемое имя источника (для логов). */
  readonly name: string;
  /** Запускает получение правил и оповещает о изменениях. */
  start(onChange: (rules: Rule[]) => void): void | Promise<void>;
  /** Останавливает получение правил. */
  stop(): void;
}
