import { IMqttAdapter, MqttSubscribeOptions, MqttPublishOptions, MqttPacket } from '../src/mqtt';

/**
 * Мок-адаптер MQTT для тестирования.
 * Имитирует подключение, подписки и публикации, позволяет симулировать входящие сообщения.
 */
export class MockMqttAdapter implements IMqttAdapter {
  private connected = false;
  private subscriptions = new Set<string>();
  private messageCallback: ((topic: string, payload: Buffer, packet: MqttPacket) => void) | null = null;
  private publishedMessages: Array<{ topic: string; payload: string | Buffer; options?: MqttPublishOptions }> = [];

  async connect(): Promise<void> {
    this.connected = true;
    console.log('[MockMqttAdapter] Подключён');
  }

  async disconnect(): Promise<void> {
    this.connected = false;
    this.subscriptions.clear();
    console.log('[MockMqttAdapter] Отключён');
  }

  async subscribe(topic: string | string[], _options?: MqttSubscribeOptions): Promise<void> {
    this.ensureConnected();
    const topics = Array.isArray(topic) ? topic : [topic];
    topics.forEach((t) => this.subscriptions.add(t));
    console.log(`[MockMqttAdapter] Подписан на: ${topics.join(', ')}`);
  }

  async unsubscribe(topic: string | string[]): Promise<void> {
    this.ensureConnected();
    const topics = Array.isArray(topic) ? topic : [topic];
    topics.forEach((t) => this.subscriptions.delete(t));
    console.log(`[MockMqttAdapter] Отписан от: ${topics.join(', ')}`);
  }

  async publish(topic: string, payload: string | Buffer, options?: MqttPublishOptions): Promise<void> {
    this.ensureConnected();
    this.publishedMessages.push({ topic, payload, options });
    console.log(`[MockMqttAdapter] Опубликовано в ${topic}: ${payload.toString().substring(0, 50)}...`);
  }

  onMessage(callback: (topic: string, payload: Buffer, packet: MqttPacket) => void): void {
    this.messageCallback = callback;
  }

  /**
   * Симулирует получение сообщения от брокера.
   * @param topic - топик.
   * @param payload - содержимое (строка или Buffer).
   * @param qos - QoS (по умолчанию 0).
   * @param retain - флаг retain (по умолчанию false).
   */
  simulateMessage(topic: string, payload: string | Buffer, qos: 0 | 1 | 2 = 0, retain = false): void {
    if (!this.messageCallback) {
      throw new Error('Обработчик сообщений не зарегистрирован');
    }
    const packet: MqttPacket = {
      topic,
      payload: Buffer.isBuffer(payload) ? payload : Buffer.from(payload),
      qos,
      retain,
      timestamp: Date.now(),
    };
    console.log(`[MockMqttAdapter] Симуляция сообщения: ${topic} -> ${payload.toString().substring(0, 50)}...`);
    this.messageCallback(topic, packet.payload, packet);
  }

  /**
   * Возвращает список опубликованных сообщений (для проверки в тестах).
   */
  getPublishedMessages(): Array<{ topic: string; payload: string | Buffer; options?: MqttPublishOptions }> {
    return [...this.publishedMessages];
  }

  /**
   * Очищает список опубликованных сообщений.
   */
  clearPublishedMessages(): void {
    this.publishedMessages = [];
  }

  /**
   * Проверяет, подписан ли адаптер на указанный топик.
   */
  isSubscribed(topic: string): boolean {
    return this.subscriptions.has(topic);
  }

  /**
   * Возвращает множество подписок.
   */
  getSubscriptions(): Set<string> {
    return new Set(this.subscriptions);
  }

  private ensureConnected(): void {
    if (!this.connected) {
      throw new Error('MockMqttAdapter не подключён');
    }
  }
}