/**
 * Провайдер правил из MongoDB.
 * Хранилище правил: коллекция `rules` в формате rule-engine.
 * Поддерживает поле `enabled` для отключения правил.
 */

import { Collection, Document, MongoClient } from 'mongodb';

import { Rule, RuleBuilder, RuleConfig, resolveConfig } from '../config';
import { RulesProvider } from './rulesProvider';

const DEFAULT_DB_NAME = 'saspik';
const DEFAULT_COLLECTION = 'rules';

function isRuleDocument(doc: Document): boolean {
  return Boolean(doc.id && doc.trigger && Array.isArray(doc.then));
}

export class MongoRulesProvider implements RulesProvider {
  readonly name = 'mongo';
  private client: MongoClient;
  private dbName: string;
  private timer: NodeJS.Timeout | null = null;
  private onChangeCallback: (rules: Rule[]) => void = () => {};
  private currentSignature = '';

  constructor(
    private url: string,
    private pollIntervalMs = 5000,
    private collectionName: string = DEFAULT_COLLECTION,
  ) {
    this.client = new MongoClient(url, { serverSelectionTimeoutMS: 5000 });
    let dbName = DEFAULT_DB_NAME;
    try {
      const pathname = new URL(url).pathname.replace(/^\//, '');
      if (pathname) {
        dbName = pathname;
      }
    } catch {
      // ignore, используем значение по умолчанию
    }
    this.dbName = dbName;
  }

  async start(onChange: (rules: Rule[]) => void): Promise<void> {
    this.onChangeCallback = onChange;
    await this.client.connect();
    console.log(`[MongoRulesProvider] Подключено к ${this.url}`);
    await this.loadAndNotify();
    this.timer = setInterval(() => {
      this.loadAndNotify().catch((err) => {
        console.error('[MongoRulesProvider] Ошибка опроса правил:', err);
      });
    }, this.pollIntervalMs);
    console.log(`[MongoRulesProvider] Опрос правил каждые ${this.pollIntervalMs} мс`);
  }

  stop(): void {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
    this.client.close().catch(() => {});
    console.log('[MongoRulesProvider] Остановлен');
  }

  private async loadAndNotify(): Promise<void> {
    const collection: Collection<Document> = this.client
      .db(this.dbName)
      .collection(this.collectionName);

    const docs = await collection
      .find({ enabled: { $ne: false } })
      .sort({ _id: 1 })
      .toArray();

    const ruleDocs = docs.filter(isRuleDocument);
    const signature = ruleDocs
      .map((d) => JSON.stringify({ ...d, _id: String(d._id) }))
      .join('|');

    if (signature === this.currentSignature) {
      return;
    }

    const config: RuleConfig[] = ruleDocs.map((d) => this.resolveDocument(d));
    const rules = RuleBuilder.buildFromConfig({ rules: config });
    this.currentSignature = signature;
    this.onChangeCallback(rules);
    console.log(`[MongoRulesProvider] Правил загружено: ${rules.length}`);
  }

  private resolveDocument(doc: Document): RuleConfig {
    const { _id, enabled, ...rest } = doc;
    const resolved = resolveConfig(JSON.stringify(rest));
    return JSON.parse(resolved) as RuleConfig;
  }
}
