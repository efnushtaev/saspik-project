import { Action } from './action.interface';
import { MessageContext } from '../context';
import { IMqttAdapter } from '../mqtt';
import { LogLevel } from './types';

/**
 * Действие логирования в консоль.
 */
export class LogAction implements Action {
  /**
   * Создаёт действие логирования.
   * @param level - уровень логирования ('info', 'warn', 'error').
   * @param message - шаблон сообщения (может содержать `{{field}}`).
   */
  constructor(private level: LogLevel, private message: string) {}

  async execute(ctx: MessageContext, _publisher: IMqttAdapter): Promise<void> {
    const rendered = this.renderMessage(ctx);
    const timestamp = new Date(ctx.timestamp).toISOString();
    const logLine = `[${timestamp}] [${this.level.toUpperCase()}] ${rendered}`;

    switch (this.level) {
      case 'info':
        console.info(logLine);
        break;
      case 'warn':
        console.warn(logLine);
        break;
      case 'error':
        console.error(logLine);
        break;
      default:
        console.log(logLine);
    }
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