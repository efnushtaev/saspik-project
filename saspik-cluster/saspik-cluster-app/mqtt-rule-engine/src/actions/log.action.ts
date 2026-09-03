import { Action } from './action.interface';
import { MessageContext } from '../context';
import { IMqttAdapter } from '../mqtt';
import { LogLevel } from './types';
import { consoleLog, publishLog } from '../logging';

/**
 * Действие логирования: вывод в консоль (JSON-конверт) и публикация в лог-топик.
 */
export class LogAction implements Action {
  /**
   * Создаёт действие логирования.
   * @param level - уровень логирования ('info', 'warn', 'error').
   * @param message - шаблон сообщения (может содержать `{{field}}`).
   */
  constructor(private level: LogLevel, private message: string) {}

  async execute(ctx: MessageContext, publisher: IMqttAdapter): Promise<void> {
    const rendered = this.renderMessage(ctx);
    const env = {
      level: this.level,
      src: 'rule-engine',
      event: 'log',
      msg: rendered,
      topic: ctx.topic,
    };
    consoleLog(env);
    await publishLog(publisher, env);
  }

  /**
   * Заменяет плейсхолдеры `{{field}}` на значения из JSON контекста.
   */
  private renderMessage(ctx: MessageContext): string {
    return this.message.replace(/\{\{([^}]+)\}\}/g, (match, fieldName) => {
      const value = ctx.getValue(`$.${fieldName.trim()}`);
      return value !== undefined ? String(value) : match;
    });
  }
}