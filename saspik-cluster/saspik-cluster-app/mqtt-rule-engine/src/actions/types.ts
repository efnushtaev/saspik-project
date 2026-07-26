/**
 * Типы для действий.
 */

/** Уровень логирования. */
export type LogLevel = 'info' | 'warn' | 'error';

/** Структура JSON-конфигурации действия. */
export interface ActionConfig {
  /** Тип действия. */
  action: string;
  /** Параметры действия (зависят от типа). */
  params?: Record<string, any>;
}