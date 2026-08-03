import fs from 'fs';
import { RuleBuilder, Rule } from './builder';
import { RulesConfig } from './types';
import { DEFAULT_CONFIG_PATH, WATCH_INTERVAL_MS } from './constants';

const ENV_VAR_REGEX = /\$\{([^}]+)\}/g;

const EXPR_REGEX = /\$\{expr:([^}]+)\}/g;

function resolveEnvVars(value: string): string {
  return value.replace(ENV_VAR_REGEX, (_match, varName: string) => {
    const trimmed = varName.trim();
    if (trimmed.startsWith('expr:')) {
      return _match;
    }
    return process.env[trimmed] ?? _match;
  });
}

function safeEval(expr: string): number {
  const tokens = expr.replace(/\s+/g, '').split(/([+\-*/()])/).filter(Boolean);
  let pos = 0;

  function parseExpr(): number {
    let result = parseTerm();
    while (pos < tokens.length && (tokens[pos] === '+' || tokens[pos] === '-')) {
      const op = tokens[pos++];
      const right = parseTerm();
      if (op === '+') result += right;
      else result -= right;
    }
    return result;
  }

  function parseTerm(): number {
    let result = parseFactor();
    while (pos < tokens.length && (tokens[pos] === '*' || tokens[pos] === '/')) {
      const op = tokens[pos++];
      const right = parseFactor();
      if (op === '*') result *= right;
      else result /= right;
    }
    return result;
  }

  function parseFactor(): number {
    if (pos >= tokens.length) throw new Error('Неожиданный конец выражения');
    if (tokens[pos] === '(') {
      pos++;
      const result = parseExpr();
      if (pos >= tokens.length || tokens[pos] !== ')') {
        throw new Error('Ожидалась закрывающая скобка');
      }
      pos++;
      return result;
    }
    const numStr = tokens[pos++];
    const num = Number(numStr);
    if (isNaN(num)) throw new Error(`Некорректное число: ${numStr}`);
    return num;
  }

  return parseExpr();
}

function resolveExprs(value: string): string {
  return value.replace(EXPR_REGEX, (_match, exprContent: string) => {
    const resolved = exprContent.replace(/([A-Z][A-Z0-9_]+)/g, (varName: string) => {
      return process.env[varName] ?? varName;
    });
    try {
      return String(safeEval(resolved));
    } catch (err) {
      console.warn(`[ConfigWatcher] failed to evaluate expression "${resolved}":`, err);
      return _match;
    }
  });
}

export function resolveConfig(content: string): string {
  let result = resolveEnvVars(content);
  result = resolveExprs(result);
  return result;
}

export class ConfigWatcher {
  readonly name = 'file';
  private currentContent: string = '';
  private watcher: fs.StatWatcher | null = null;
  private onChangeCallback: (rules: Rule[]) => void = () => {};

  constructor(private configPath: string = DEFAULT_CONFIG_PATH) {}

  start(onChange: (rules: Rule[]) => void): void {
    this.onChangeCallback = onChange;
    this.loadAndNotify().catch((err) => {
      console.error('Ошибка при начальной загрузке конфигурации:', err);
    });
    this.watcher = fs.watchFile(this.configPath, { interval: WATCH_INTERVAL_MS }, () => {
      this.loadAndNotify().catch((err) => {
        console.error('Ошибка при перезагрузке конфигурации:', err);
      });
    });
    console.log(`Наблюдение за файлом ${this.configPath} запущено`);
  }

  stop(): void {
    if (this.watcher) {
      fs.unwatchFile(this.configPath);
      this.watcher = null;
      console.log('Наблюдение остановлено');
    }
  }

  private async loadAndNotify(): Promise<void> {
    try {
      const content = await fs.promises.readFile(this.configPath, 'utf-8');
      if (content === this.currentContent) {
        return;
      }
      console.log('Обнаружено изменение конфигурации, перезагружаем...');
      const resolvedContent = resolveConfig(content);
      const config: RulesConfig = JSON.parse(resolvedContent);
      const rules = RuleBuilder.buildFromConfig(config);
      this.currentContent = content;
      this.onChangeCallback(rules);
      console.log(`Конфигурация перезагружена, правил: ${rules.length}`);
    } catch (err) {
      console.error('Не удалось загрузить или распарсить конфигурацию:', err);
    }
  }
}
