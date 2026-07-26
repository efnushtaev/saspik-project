/**
 * Основные типы движка правил.
 */

import { Rule } from '../config';

/** Индекс правил по топику. */
export type RuleIndex = Map<string, Rule[]>;