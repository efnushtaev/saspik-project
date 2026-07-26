/**
 * Константы для условий.
 */

/** Допустимые операторы сравнения. */
export const COMPARISON_OPERATORS = ['==', '!=', '>', '>=', '<', '<='] as const;

/** Регулярное выражение для парсинга JSONPath-условия. */
export const JSONPATH_CONDITION_REGEX = /^\$\.([a-zA-Z0-9_.]+)\s*(==|!=|>=|<=|>|<)\s*(.+)$/;

/** Имена типов условий. */
export const CONDITION_TYPES = {
  TOPIC_REGEX: 'topicRegex',
  JSONPATH: 'jsonpath',
  PAYLOAD_EQUALS: 'payloadEquals',
  AND: 'and',
  OR: 'or',
  NOT: 'not',
  TIMEBETWEEN: 'timeBetween',
} as const;