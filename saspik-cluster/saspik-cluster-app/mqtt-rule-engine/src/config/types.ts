/**
 * Типы для конфигурации правил.
 */

import { ActionConfig } from '../actions';

/** Триггер правила (подписка на топик). */
export interface TriggerConfig {
  /** Топик или массив топиков (может содержать wildcards +, #). */
  topic: string | string[];
  /** QoS подписки (по умолчанию 0). */
  qos?: 0 | 1 | 2;
}

/** Конфигурация одного правила. */
export interface RuleConfig {
  /** Уникальный идентификатор правила. */
  id: string;
  /** Триггер. */
  trigger: TriggerConfig;
  /** Условие (может быть сложным объектом). */
  when?: any;
  /** Массив действий. */
  then: ActionConfig[];
}

/** Корневая структура JSON-конфигурации. */
export interface RulesConfig {
  /** Массив правил. */
  rules: RuleConfig[];
}