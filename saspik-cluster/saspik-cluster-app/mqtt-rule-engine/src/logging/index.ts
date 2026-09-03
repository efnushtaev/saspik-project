import { IMqttAdapter } from '../mqtt';

/**
 * Единый JSON-конверт лога (согласован для device/server/rule-engine).
 * Теги в Influx: topic, level, event, src, ...
 */
export interface LogEnvelope {
  level: 'debug' | 'info' | 'warn' | 'error';
  src: string;
  event: string;
  msg: string;
  topic?: string;
  [key: string]: string | number | boolean | null | undefined;
}

/** Топик логов rule-engine (подпадает под pattern rule-engine/+/log в telegraf). */
const RULE_ENGINE_LOG_TOPIC = 'rule-engine/worker/log';

/** Компонент-источник. */
const SRC = 'rule-engine';

/**
 * Выводит конверт лога в консоль (stdout) в JSON-виде.
 * @param env - конверт лога.
 */
export function consoleLog(env: LogEnvelope): void {
  const line = JSON.stringify(env);
  switch (env.level) {
    case 'error':
      console.error(line);
      break;
    case 'warn':
      console.warn(line);
      break;
    case 'debug':
      // debug в stdout, чтобы не смешивать с ошибками
      if (typeof console.debug === 'function') {
        console.debug(line);
      } else {
        console.log(line);
      }
      break;
    default:
      console.log(line);
  }
}

/**
 * Публикует конверт лога в MQTT-топик rule-engine (для telegraf → Influx).
 * Ошибки публикации не бросаем — логирование не должно ломать основную логику.
 * @param adapter - MQTT-адаптер.
 * @param env - конверт лога.
 */
export async function publishLog(adapter: IMqttAdapter, env: LogEnvelope): Promise<void> {
  const payload = JSON.stringify(env);
  try {
    await adapter.publish(RULE_ENGINE_LOG_TOPIC, payload, { qos: 0, retain: false });
  } catch {
    // Игнорируем — лог не должен прерывать поток правил
  }
}

/**
 * Удобная обёртка: логирует в консоль И публикует в MQTT.
 * @param adapter - MQTT-адаптер.
 * @param partial - частичный конверт (src/event/topic заполняются автоматически).
 */
export async function log(adapter: IMqttAdapter, partial: Partial<LogEnvelope> & { msg: string }): Promise<void> {
  const env: LogEnvelope = {
    level: partial.level ?? 'info',
    src: SRC,
    event: partial.event ?? 'log',
    msg: partial.msg,
    topic: partial.topic,
  };
  consoleLog(env);
  await publishLog(adapter, env);
}
