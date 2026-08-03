/**
 * Провайдер правил из HTTP API сервера.
 * Ожидает JSON-ответ вида { "rules": [...] } или [...]
 * в формате rule-engine. Поддерживает поле `enabled`.
 */

import { Rule, RuleBuilder, RuleConfig, resolveConfig } from '../config';
import { RulesProvider } from './rulesProvider';

function isRuleDocument(doc: unknown): doc is Record<string, unknown> {
  return Boolean(
    doc &&
      typeof doc === 'object' &&
      (doc as Record<string, unknown>).id &&
      (doc as Record<string, unknown>).trigger &&
      Array.isArray((doc as Record<string, unknown>).then),
  );
}

export class ApiRulesProvider implements RulesProvider {
  readonly name = 'api';
  private timer: NodeJS.Timeout | null = null;
  private onChangeCallback: (rules: Rule[]) => void = () => {};
  private currentSignature = '';

  constructor(private url: string, private pollIntervalMs = 5000) {}

  async start(onChange: (rules: Rule[]) => void): Promise<void> {
    this.onChangeCallback = onChange;
    await this.loadAndNotify();
    this.timer = setInterval(() => {
      this.loadAndNotify().catch((err) => {
        console.error('[ApiRulesProvider] Ошибка опроса правил:', err);
      });
    }, this.pollIntervalMs);
    console.log(`[ApiRulesProvider] Опрос правил каждые ${this.pollIntervalMs} мс`);
  }

  stop(): void {
    if (this.timer) {
      clearInterval(this.timer);
      this.timer = null;
    }
    console.log('[ApiRulesProvider] Остановлен');
  }

  private async loadAndNotify(): Promise<void> {
    const res = await fetch(this.url);
    if (!res.ok) {
      throw new Error(`[ApiRulesProvider] HTTP ${res.status} при запросе ${this.url}`);
    }

    const body: unknown = await res.json();
    const raw = Array.isArray(body) ? body : (body as Record<string, unknown>)?.rules;
    const arr = Array.isArray(raw) ? raw : [];
    const docs = arr.filter(isRuleDocument).filter(
      (d) => d.enabled !== false,
    );

    const signature = JSON.stringify(docs);
    if (signature === this.currentSignature) {
      return;
    }

    const config: RuleConfig[] = docs.map((d) =>
      JSON.parse(resolveConfig(JSON.stringify(d))) as RuleConfig,
    );
    const rules = RuleBuilder.buildFromConfig({ rules: config });
    this.currentSignature = signature;
    this.onChangeCallback(rules);
    console.log(`[ApiRulesProvider] Правил загружено: ${rules.length}`);
  }
}
