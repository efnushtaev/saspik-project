import mqtt, { MqttClient, IClientOptions, ISubscriptionGrant, IPublishPacket } from 'mqtt';
import { IMqttAdapter, MqttSubscribeOptions, MqttPublishOptions, MqttPacket } from './types';
import { DEFAULT_BROKER_URL, DEFAULT_CONNECT_OPTIONS } from './constants';

/**
 * Реализация MQTT-адаптера на основе библиотеки mqtt.
 */
export class MqttAdapter implements IMqttAdapter {
  private client: MqttClient | null = null;
  private messageCallback: ((topic: string, payload: Buffer, packet: MqttPacket) => void) | null = null;
  private isConnected = false;

  /**
   * Создаёт экземпляр адаптера.
   * @param brokerUrl - URL брокера (по умолчанию 'mqtt://localhost:1883').
   * @param options - дополнительные опции подключения.
   */
  constructor(
    private brokerUrl: string = DEFAULT_BROKER_URL,
    private options: IClientOptions = DEFAULT_CONNECT_OPTIONS
  ) {}

  /**
   * Подключиться к брокеру.
   */
  async connect(): Promise<void> {
    if (this.client) {
      throw new Error('MQTT клиент уже создан');
    }

    return new Promise((resolve, reject) => {
      try {
        this.client = mqtt.connect(this.brokerUrl, this.options);

        this.client.on('connect', () => {
          this.isConnected = true;
          console.log(`MQTT подключён к ${this.brokerUrl}`);
          resolve();
        });

        this.client.on('error', (err) => {
          console.error('MQTT ошибка:', err);
          reject(err);
        });

        this.client.on('message', (topic: string, payload: Buffer, packet: IPublishPacket) => {
          if (this.messageCallback) {
            const mqttPacket: MqttPacket = {
              topic,
              payload,
              qos: packet.qos as 0 | 1 | 2,
              retain: packet.retain,
              timestamp: Date.now(),
            };
            this.messageCallback(topic, payload, mqttPacket);
          }
        });

        this.client.on('close', () => {
          this.isConnected = false;
          console.log('MQTT соединение закрыто');
        });
      } catch (err) {
        reject(err);
      }
    });
  }

  /**
   * Отключиться от брокера.
   */
  async disconnect(): Promise<void> {
    if (!this.client) {
      return;
    }

    return new Promise((resolve) => {
      this.client!.end(false, () => {
        this.client = null;
        this.isConnected = false;
        resolve();
      });
    });
  }

  /**
   * Подписаться на топик(и).
   */
  async subscribe(topic: string | string[], options?: MqttSubscribeOptions): Promise<void> {
    this.ensureConnected();

    const opts = { qos: options?.qos ?? 0 };
    return new Promise((resolve, reject) => {
      this.client!.subscribe(topic, opts, (err: Error | null, _granted?: ISubscriptionGrant[]) => {
        if (err) {
          reject(err);
        } else {
          const topics = Array.isArray(topic) ? topic : [topic];
          console.log(`Подписан на топики: ${topics.join(', ')}`);
          resolve();
        }
      });
    });
  }

  /**
   * Отписаться от топика(ов).
   */
  async unsubscribe(topic: string | string[]): Promise<void> {
    this.ensureConnected();

    return new Promise((resolve, reject) => {
      this.client!.unsubscribe(topic, (err?: Error) => {
        if (err) {
          reject(err);
        } else {
          const topics = Array.isArray(topic) ? topic : [topic];
          console.log(`Отписан от топиков: ${topics.join(', ')}`);
          resolve();
        }
      });
    });
  }

  /**
   * Опубликовать сообщение.
   */
  async publish(topic: string, payload: string | Buffer, options?: MqttPublishOptions): Promise<void> {
    this.ensureConnected();

    const opts = {
      qos: options?.qos ?? 0,
      retain: options?.retain ?? false,
    };

    return new Promise((resolve, reject) => {
      this.client!.publish(topic, payload, opts, (err?: Error) => {
        if (err) {
          reject(err);
        } else {
          console.log(`Опубликовано в ${topic}: ${payload.toString().substring(0, 200)}`);
          resolve();
        }
      });
    });
  }

  /**
   * Зарегистрировать обработчик входящих сообщений.
   */
  onMessage(callback: (topic: string, payload: Buffer, packet: MqttPacket) => void): void {
    this.messageCallback = callback;
  }

  /**
   * Проверяет, что клиент подключён.
   * @throws {Error} Если клиент не подключён.
   */
  private ensureConnected(): void {
    if (!this.client || !this.isConnected) {
      throw new Error('MQTT клиент не подключён');
    }
  }
}