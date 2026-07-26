import { MessageContextData } from './types';

/**
 * Контекст MQTT-сообщения.
 * Предоставляет удобные методы для работы с payload (JSON, извлечение значений).
 */
export class MessageContext {
  private parsedJson: any | null = null;
  private jsonParseError: Error | null = null;

  /**
   * Создаёт контекст из данных сообщения.
   * @param data - данные сообщения.
   */
  constructor(private data: MessageContextData) {}

  /** Топик сообщения. */
  get topic(): string {
    return this.data.topic;
  }

  /** Сырой payload (Buffer). */
  get rawPayload(): Buffer {
    return this.data.rawPayload;
  }

  /** QoS сообщения. */
  get qos(): 0 | 1 | 2 {
    return this.data.qos;
  }

  /** Флаг retain. */
  get retain(): boolean {
    return this.data.retain;
  }

  /** Временная метка получения сообщения (мс). */
  get timestamp(): number {
    return this.data.timestamp;
  }

  /**
   * Возвращает payload как строку (UTF-8).
   */
  get payloadString(): string {
    return this.rawPayload.toString('utf-8');
  }

  /**
   * Лениво парсит payload как JSON и возвращает объект.
   * При ошибке парсинга возвращает null и сохраняет ошибку.
   */
  get json(): any {
    if (this.parsedJson !== null) {
      return this.parsedJson;
    }
    if (this.jsonParseError !== null) {
      return null;
    }

    try {
      const text = this.payloadString.trim();
      if (!text) {
        this.parsedJson = null;
        return null;
      }
      this.parsedJson = JSON.parse(text);
      return this.parsedJson;
    } catch (err) {
      this.jsonParseError = err as Error;
      return null;
    }
  }

  /**
   * Извлекает значение из JSON payload по упрощённому JSONPath.
   * Поддерживает только выражения вида `$.field.subfield`.
   * @param path - строка JSONPath (например, "$.temperature").
   * @returns извлечённое значение или undefined, если путь не найден или JSON невалиден.
   */
  getValue(path: string): any {
    const obj = this.json;
    if (obj === null) {
      return undefined;
    }

    // Упрощённый парсинг JSONPath: удаляем "$." и разбиваем по точкам.
    if (!path.startsWith('$.')) {
      return undefined;
    }
    const parts = path.substring(2).split('.');
    let current = obj;
    for (const part of parts) {
      if (current === null || typeof current !== 'object' || !(part in current)) {
        return undefined;
      }
      current = current[part];
    }
    return current;
  }

  /**
   * Создаёт контекст из пакета MQTT (как передаётся в адаптере).
   * @param topic - топик.
   * @param payload - сырые данные.
   * @param packet - пакет MQTT (с qos, retain).
   */
  static fromMqttPacket(topic: string, payload: Buffer, packet: { qos: 0 | 1 | 2; retain: boolean }): MessageContext {
    const data: MessageContextData = {
      topic,
      rawPayload: payload,
      qos: packet.qos,
      retain: packet.retain,
      timestamp: Date.now(),
    };
    return new MessageContext(data);
  }
}