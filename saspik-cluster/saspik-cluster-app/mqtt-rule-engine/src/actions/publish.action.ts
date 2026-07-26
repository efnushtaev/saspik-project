import { Action } from './action.interface';
import { MessageContext } from '../context';
import { IMqttAdapter, MqttPublishOptions } from '../mqtt';
import { DEFAULT_PUBLISH_QOS, DEFAULT_PUBLISH_RETAIN } from './constants';

/**
 * Действие публикации сообщения в MQTT-топик.
 * Поддерживает шаблонизацию payload: строки вида `{{fieldName}}` заменяются на значения из JSON контекста.
 */
export class PublishAction implements Action {
  private topic: string;
  private payloadTemplate: string;
  private options: MqttPublishOptions;

  /**
   * Создаёт действие публикации.
   * @param topic - топик для публикации.
   * @param payload - шаблон payload (может содержать `{{field}}`).
   * @param options - опции публикации (qos, retain).
   */
  constructor(topic: string, payload: string, options?: MqttPublishOptions) {
    this.topic = topic;
    this.payloadTemplate = payload;
    this.options = {
      qos: options?.qos ?? DEFAULT_PUBLISH_QOS,
      retain: options?.retain ?? DEFAULT_PUBLISH_RETAIN,
    };
  }

  async execute(ctx: MessageContext, publisher: IMqttAdapter): Promise<void> {
    const payload = this.renderPayload(ctx);
    await publisher.publish(this.topic, payload, this.options);
  }

  /**
   * Заменяет плейсхолдеры `{{field}}` на значения из JSON контекста.
   * Если поле отсутствует, плейсхолдер остаётся без изменений.
   */
  private renderPayload(ctx: MessageContext): string {
    return this.payloadTemplate.replace(/\{\{([^}]+)\}\}/g, (match, fieldName) => {
      const value = ctx.getValue(`$.${fieldName.trim()}`);
      return value !== undefined ? String(value) : match;
    });
  }
}