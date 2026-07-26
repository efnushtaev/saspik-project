import { IMqttAdapter } from '../mqtt';
import { MessageContext } from '../context';
import { Rule } from '../config';
import { RuleIndex } from './types';
import { topicMatches } from '../utils';

/**
 * Движок правил MQTT.
 * Управляет подписками, оценивает условия и выполняет действия.
 */
export class RuleEngine {
  private rules: Rule[] = [];
  private index: RuleIndex = new Map();
  private subscribedTopics: Set<string> = new Set();

  /**
   * Создаёт движок.
   * @param adapter - MQTT-адаптер для подписки и публикации.
   */
  constructor(private adapter: IMqttAdapter) {
    // Регистрируем обработчик входящих сообщений
    this.adapter.onMessage((topic, payload, packet) => {
      this.handleMessage(topic, payload, packet).catch((err) => {
        console.error('Ошибка при обработке сообщения:', err);
      });
    });
  }

  /**
   * Загружает новые правила, перестраивает индекс и обновляет подписки.
   * @param newRules - массив правил.
   */
  async loadRules(newRules: Rule[]): Promise<void> {
    console.log(`Загрузка ${newRules.length} правил...`);

    // Отписываемся от старых топиков, которые больше не нужны
    await this.unsubscribeUnused(newRules);

    // Обновляем внутренние структуры
    this.rules = newRules;
    this.rebuildIndex();

    // Подписываемся на новые топики
    await this.subscribeToNewTopics(newRules);

    console.log(`Правила загружены. Индекс содержит ${this.index.size} топик(ов).`);
  }

  /**
   * Обрабатывает входящее MQTT-сообщение.
   */
  private async handleMessage(topic: string, payload: Buffer, packet: any): Promise<void> {
    const ctx = MessageContext.fromMqttPacket(topic, payload, packet);
    const candidates = this.findCandidates(topic);

    console.log(`Получено сообщение из топика ${topic}: ${payload.toString('utf-8').trim()}`);
    for (const rule of candidates) {
      try {
        const conditionPassed = rule.condition ? rule.condition.evaluate(ctx) : true;
        if (conditionPassed) {
          console.log(`Правило ${rule.id} выполнено`);
          await this.executeActions(rule.actions, ctx);
        }
      } catch (err) {
        console.error(`Ошибка при выполнении правила ${rule.id}:`, err);
      }
    }
  }

  /**
   * Находит правила, подходящие для данного топика (с учётом wildcards).
   */
  private findCandidates(topic: string): Rule[] {
    const candidates: Rule[] = [];

    // Сначала точное совпадение
    const exact = this.index.get(topic);
    if (exact) {
      candidates.push(...exact);
    }

    // Затем перебор всех шаблонов с wildcards
    for (const [pattern, rules] of this.index.entries()) {
      if (pattern === topic) continue; // уже добавлено
      if (topicMatches(pattern, topic)) {
        candidates.push(...rules);
      }
    }

    return candidates;
  }

  /**
   * Выполняет все действия правила.
   */
  private async executeActions(actions: any[], ctx: MessageContext): Promise<void> {
    // Можно выполнять параллельно, но сохраняем порядок для простоты
    for (const action of actions) {
      try {
        await action.execute(ctx, this.adapter);
      } catch (err) {
        console.error('Ошибка при выполнении действия:', err);
      }
    }
  }

  /**
   * Перестраивает индекс правил (топик → список правил).
   */
  private rebuildIndex(): void {
    this.index.clear();

    for (const rule of this.rules) {
      const topics = Array.isArray(rule.trigger.topic)
        ? rule.trigger.topic
        : [rule.trigger.topic];

      for (const topic of topics) {
        if (!this.index.has(topic)) {
          this.index.set(topic, []);
        }
        this.index.get(topic)!.push(rule);
      }
    }
  }

  /**
   * Отписывается от топиков, которые больше не используются.
   */
  private async unsubscribeUnused(newRules: Rule[]): Promise<void> {
    const newTopics = new Set<string>();
    for (const rule of newRules) {
      const topics = Array.isArray(rule.trigger.topic)
        ? rule.trigger.topic
        : [rule.trigger.topic];
      topics.forEach((t) => newTopics.add(t));
    }

    const toUnsubscribe = Array.from(this.subscribedTopics).filter(
      (t) => !newTopics.has(t)
    );

    for (const topic of toUnsubscribe) {
      try {
        await this.adapter.unsubscribe(topic);
        this.subscribedTopics.delete(topic);
      } catch (err) {
        console.error(`Не удалось отписаться от топика ${topic}:`, err);
      }
    }
  }

  /**
   * Подписывается на топики новых правил.
   */
  private async subscribeToNewTopics(newRules: Rule[]): Promise<void> {
    const topics = new Set<string>();
    for (const rule of newRules) {
      const ruleTopics = Array.isArray(rule.trigger.topic)
        ? rule.trigger.topic
        : [rule.trigger.topic];
      ruleTopics.forEach((t) => topics.add(t));
    }

    for (const topic of topics) {
      if (this.subscribedTopics.has(topic)) {
        continue; // уже подписаны
      }
      try {
        const qos = newRules.find((r) => {
          const topics = Array.isArray(r.trigger.topic) ? r.trigger.topic : [r.trigger.topic];
          return topics.includes(topic);
        })?.trigger.qos ?? 0;

        await this.adapter.subscribe(topic, { qos });
        this.subscribedTopics.add(topic);
      } catch (err) {
        console.error(`Не удалось подписаться на топик ${topic}:`, err);
      }
    }
  }
}