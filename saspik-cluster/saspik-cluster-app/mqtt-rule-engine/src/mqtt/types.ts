/**
 * Интерфейс MQTT-адаптера.
 * Абстракция над клиентом MQTT, позволяющая подписываться, отписываться, публиковать сообщения.
 */
export interface IMqttAdapter {
  /**
   * Подключиться к брокеру MQTT.
   * @returns Promise, разрешающийся после успешного подключения.
   */
  connect(): Promise<void>;

  /**
   * Отключиться от брокера.
   * @returns Promise, разрешающийся после отключения.
   */
  disconnect(): Promise<void>;

  /**
   * Подписаться на топик (или несколько топиков).
   * @param topic - строка топика или массив строк.
   * @param options - опции подписки (например, QoS).
   * @returns Promise, разрешающийся после успешной подписки.
   */
  subscribe(topic: string | string[], options?: MqttSubscribeOptions): Promise<void>;

  /**
   * Отписаться от топика (или нескольких топиков).
   * @param topic - строка топика или массив строк.
   * @returns Promise, разрешающийся после успешной отписки.
   */
  unsubscribe(topic: string | string[]): Promise<void>;

  /**
   * Опубликовать сообщение в топик.
   * @param topic - топик для публикации.
   * @param payload - содержимое сообщения (строка или Buffer).
   * @param options - опции публикации (QoS, retain).
   * @returns Promise, разрешающийся после успешной публикации.
   */
  publish(topic: string, payload: string | Buffer, options?: MqttPublishOptions): Promise<void>;

  /**
   * Зарегистрировать обработчик входящих сообщений.
   * @param callback - функция, вызываемая при получении сообщения.
   */
  onMessage(callback: (topic: string, payload: Buffer, packet: MqttPacket) => void): void;
}

/**
 * Опции подписки MQTT.
 */
export interface MqttSubscribeOptions {
  /** Уровень качества обслуживания (0, 1, 2). */
  qos?: 0 | 1 | 2;
}

/**
 * Опции публикации MQTT.
 */
export interface MqttPublishOptions {
  /** Уровень качества обслуживания (0, 1, 2). */
  qos?: 0 | 1 | 2;
  /** Флаг retain. */
  retain?: boolean;
}

/**
 * Пакет MQTT, переданный библиотекой mqtt.
 */
export interface MqttPacket {
  /** Топик сообщения. */
  topic: string;
  /** Полезная нагрузка (Buffer). */
  payload: Buffer;
  /** Уровень качества обслуживания. */
  qos: 0 | 1 | 2;
  /** Флаг retain. */
  retain: boolean;
  /** Время получения (если есть). */
  timestamp?: number;
}