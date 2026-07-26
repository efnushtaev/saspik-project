/**
 * Константы для MQTT-адаптера.
 */

/** URL брокера по умолчанию. */
export const DEFAULT_BROKER_URL = 'mqtt://localhost:1883';

/** Опции подключения по умолчанию. */
export const DEFAULT_CONNECT_OPTIONS = {
  keepalive: 60,
  reconnectPeriod: 1000,
  connectTimeout: 30 * 1000,
  clean: true,
};

/** QoS по умолчанию для подписки. */
export const DEFAULT_SUBSCRIBE_QOS: 0 | 1 | 2 = 0;

/** QoS по умолчанию для публикации. */
export const DEFAULT_PUBLISH_QOS: 0 | 1 | 2 = 0;

/** Флаг retain по умолчанию. */
export const DEFAULT_RETAIN = false;