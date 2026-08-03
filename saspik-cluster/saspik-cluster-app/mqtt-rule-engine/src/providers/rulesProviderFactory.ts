/**
 * Фабрика источника правил.
 * Выбор источника через переменную окружения RULES_SOURCE:
 *  - mongo (по умолчанию) — MongoDB, коллекция rules;
 *  - api — HTTP API сервера (GET {RULES_API_URL});
 *  - file — локальный rules.json (запасной вариант, ConfigWatcher).
 */

import { ConfigWatcher, DEFAULT_CONFIG_PATH } from '../config';
import { ApiRulesProvider } from './apiRulesProvider';
import { MongoRulesProvider } from './mongoRulesProvider';
import { RulesProvider } from './rulesProvider';

export const RULES_POLL_INTERVAL_MS =
  Number(process.env.RULES_POLL_INTERVAL_MS) || 5000;

export function createRulesProvider(): RulesProvider {
  const source = process.env.RULES_SOURCE || 'mongo';

  switch (source) {
    case 'api': {
      const url = process.env.RULES_API_URL;
      if (!url) {
        throw new Error('RULES_API_URL is required when RULES_SOURCE=api');
      }
      return new ApiRulesProvider(url, RULES_POLL_INTERVAL_MS);
    }

    case 'file':
      return new ConfigWatcher(process.env.CONFIG_PATH || DEFAULT_CONFIG_PATH);

    case 'mongo':
    default: {
      const url = process.env.MONGODB_URL;
      if (!url) {
        throw new Error('MONGODB_URL is required when RULES_SOURCE=mongo');
      }
      return new MongoRulesProvider(url, RULES_POLL_INTERVAL_MS);
    }
  }
}
