/**
 * Константы для действий.
 */

/** Значения по умолчанию для публикации. */
export const DEFAULT_PUBLISH_QOS: 0 | 1 | 2 = 0;
export const DEFAULT_PUBLISH_RETAIN = false;

/** Уровни логирования. */
export const LOG_LEVELS = ['info', 'warn', 'error'] as const;

/** Имена типов действий. */
export const ACTION_TYPES = {
  PUBLISH: 'publish',
  LOG: 'log',
  TIMEOUT: 'timeout',
} as const;