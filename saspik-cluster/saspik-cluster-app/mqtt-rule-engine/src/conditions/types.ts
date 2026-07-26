/**
 * Типы для условий.
 */

/** Операторы сравнения для числовых значений. */
export type ComparisonOperator = '==' | '!=' | '>' | '>=' | '<' | '<=';

/** Структура JSON-конфигурации условия. */
export interface ConditionConfig {
  /** Тип условия. */
  type: string;
  /** Параметры условия (зависят от типа). */
  params?: Record<string, any>;
}