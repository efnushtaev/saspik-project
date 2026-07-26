import { Condition } from '../conditions';
import { Action } from '../actions';
import { TopicRegexCondition, JsonPathCondition, PayloadEqualsCondition, TimeBetweenCondition, AndCondition, OrCondition, NotCondition } from '../conditions';
import { PublishAction, LogAction, TimeoutAction } from '../actions';
import { RuleConfig, TriggerConfig } from './types';
import { CONDITION_TYPES } from '../conditions/constants';
import { ACTION_TYPES } from '../actions/constants';

/**
 * Правило, готовое к использованию в движке.
 */
export interface Rule {
  /** Идентификатор. */
  id: string;
  /** Триггер (подписка). */
  trigger: TriggerConfig;
  /** Условие (может быть null, тогда всегда истинно). */
  condition: Condition | null;
  /** Действия. */
  actions: Action[];
}

/**
 * Строитель правил из JSON-конфигурации.
 */
export class RuleBuilder {
  /**
   * Строит массив правил из конфигурации.
   * @param config - корневая конфигурация.
   * @returns массив правил.
   */
  static buildFromConfig(config: { rules: RuleConfig[] }): Rule[] {
    return config.rules.map((ruleConfig) => this.buildRule(ruleConfig));
  }

  /**
   * Строит одно правило.
   */
  private static buildRule(ruleConfig: RuleConfig): Rule {
    const condition = ruleConfig.when
      ? this.buildCondition(ruleConfig.when)
      : null;

    const actions = ruleConfig.then.map((actionConfig) =>
      this.buildAction(actionConfig)
    );

    return {
      id: ruleConfig.id,
      trigger: ruleConfig.trigger,
      condition,
      actions,
    };
  }

  /**
   * Рекурсивно строит условие из JSON-объекта.
   */
  private static buildCondition(obj: any): Condition {
    if (typeof obj !== 'object' || obj === null) {
      throw new Error(`Некорректный формат условия: ${JSON.stringify(obj)}`);
    }

    const keys = Object.keys(obj);
    if (keys.length !== 1) {
      throw new Error(`Условие должно содержать ровно один ключ: ${keys.join(',')}`);
    }

    const type = keys[0];
    const params = obj[type];

    switch (type) {
      case CONDITION_TYPES.TOPIC_REGEX:
        return new TopicRegexCondition(params as string);

      case CONDITION_TYPES.JSONPATH:
        return new JsonPathCondition(params as string);

      case CONDITION_TYPES.PAYLOAD_EQUALS:
        return new PayloadEqualsCondition(params as string);

      case CONDITION_TYPES.AND:
        if (!Array.isArray(params)) {
          throw new Error('Параметр "and" должен быть массивом условий');
        }
        return new AndCondition(params.map((c) => this.buildCondition(c)));

      case CONDITION_TYPES.OR:
        if (!Array.isArray(params)) {
          throw new Error('Параметр "or" должен быть массивом условий');
        }
        return new OrCondition(params.map((c) => this.buildCondition(c)));

      case CONDITION_TYPES.NOT:
        return new NotCondition(this.buildCondition(params));

      case CONDITION_TYPES.TIMEBETWEEN: {
        const { start, end, field } = params as { start: number; end: number; field?: string };
        if (start === undefined || end === undefined) {
          throw new Error('Условие "timeBetween" требует start и end');
        }
        return new TimeBetweenCondition({ start, end, field });
      }

      default:
        throw new Error(`Неизвестный тип условия: ${type}`);
    }
  }

  /**
   * Строит действие из JSON-объекта.
   */
  private static buildAction(config: { action: string; params?: Record<string, any> }): Action {
    const { action, params = {} } = config;

    switch (action) {
      case ACTION_TYPES.PUBLISH: {
        const { topic, payload, qos, retain } = params;
        if (!topic || !payload) {
          throw new Error('Действие "publish" требует topic и payload');
        }
        return new PublishAction(topic, payload, { qos, retain });
      }

      case ACTION_TYPES.LOG: {
        const { level = 'info', message } = params;
        if (!message) {
          throw new Error('Действие "log" требует message');
        }
        return new LogAction(level, message);
      }

      case ACTION_TYPES.TIMEOUT: {
        const { delay, then: thenActions } = params as { delay: number; then: Record<string, any>[] };
        if (delay === undefined || !Array.isArray(thenActions)) {
          throw new Error('Действие "timeout" требует delay и then (массив действий)');
        }
        const childActions = thenActions.map((a) => this.buildAction(a as { action: string; params?: Record<string, any> }));
        return new TimeoutAction(delay, childActions);
      }

      default:
        throw new Error(`Неизвестный тип действия: ${action}`);
    }
  }
}