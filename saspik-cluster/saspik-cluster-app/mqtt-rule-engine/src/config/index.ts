/**
 * Экспорт публичного API модуля конфигурации.
 */

export { RuleBuilder, Rule } from './builder';
export { ConfigWatcher, resolveConfig } from './loader';
export { RulesConfig, RuleConfig, TriggerConfig } from './types';
export { DEFAULT_CONFIG_FILE, DEFAULT_CONFIG_PATH } from './constants';