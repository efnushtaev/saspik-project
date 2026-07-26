/**
 * Типы для контекста сообщения.
 */

/** Контекст входящего MQTT-сообщения. */
export interface MessageContextData {
  /** Топик сообщения. */
  topic: string;
  /** Сырые данные payload (Buffer). */
  rawPayload: Buffer;
  /** Уровень качества обслуживания (0, 1, 2). */
  qos: 0 | 1 | 2;
  /** Флаг retain. */
  retain: boolean;
  /** Временная метка получения сообщения (мс). */
  timestamp: number;
}