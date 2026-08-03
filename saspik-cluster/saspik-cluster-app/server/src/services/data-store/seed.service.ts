import fs from "fs";
import path from "path";
import { inject, injectable } from "inversify";
import { Db } from "mongodb";

import { Mongo } from ".";
import { TYPES } from "../../types";
import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { unitsConfig } from "../../data/units.config";
import { RuleEntity } from "./types";

@injectable()
export class SeedService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
    @inject(TYPES.Mongo) private mongo: Mongo,
  ) {}

  async seed(): Promise<void> {
    const db = this.mongo.getDb();

    await this.seedUnits(db);
    await this.seedObjects(db);
    await this.seedRules(db);
  }

  private async seedUnits(db: Db): Promise<void> {
    const collection = db.collection("units");
    const count = await collection.countDocuments();
    if (count > 0) return;

    await collection.insertMany(
      unitsConfig.map((u) => ({ id: u.id, name: u.name, description: u.description })),
    );
    this.logger.log(`[SeedService] Seeded ${unitsConfig.length} units`);
  }

  private async seedObjects(db: Db): Promise<void> {
    const collection = db.collection("objects");
    const count = await collection.countDocuments();
    if (count > 0) return;

    const objects = unitsConfig.flatMap((unit) =>
      unit.objects.map((o) => ({ ...o, unitId: unit.id })),
    );
    await collection.insertMany(objects);
    this.logger.log(`[SeedService] Seeded ${objects.length} objects`);
  }

  private async seedRules(db: Db): Promise<void> {
    const collection = db.collection("rules");
    const count = await collection.countDocuments();
    if (count > 0) return;

    const rules = this.loadRulesSeed();
    if (rules.length === 0) {
      this.logger.warn("[SeedService] Rules seed file not found or empty, skipping");
      return;
    }

    await collection.insertMany(rules);
    this.logger.log(`[SeedService] Seeded ${rules.length} rules`);
  }

  private loadRulesSeed(): RuleEntity[] {
    const seedPath =
      this.config.get("INIT_RULES_SEED_PATH") ||
      path.resolve(process.cwd(), "data/rules.json");

    let content: string;
    try {
      content = fs.readFileSync(seedPath, "utf-8");
    } catch (error) {
      this.logger.warn(
        `[SeedService] Unable to read rules seed from ${seedPath}: ${(error as Error).message}`,
      );
      return [];
    }

    const config = JSON.parse(this.resolveConfig(content)) as {
      rules?: RuleEntity[];
    };
    const rules = Array.isArray(config.rules) ? config.rules : [];
    return rules.map((r) => ({ ...r, enabled: true }));
  }

  private resolveConfig(content: string): string {
    return this.resolveExprs(this.resolveEnvVars(content));
  }

  private resolveEnvVars(value: string): string {
    return value.replace(/\$\{([^}]+)\}/g, (match, varName: string) => {
      const trimmed = varName.trim();
      if (trimmed.startsWith("expr:")) {
        return match;
      }
      return process.env[trimmed] ?? match;
    });
  }

  private resolveExprs(value: string): string {
    return value.replace(/\$\{expr:([^}]+)\}/g, (_match, exprContent: string) => {
      const resolved = exprContent.replace(
        /([A-Z][A-Z0-9_]+)/g,
        (varName: string) => process.env[varName] ?? varName,
      );
      try {
        return String(this.safeEval(resolved));
      } catch {
        return _match;
      }
    });
  }

  private safeEval(expr: string): number {
    const tokens = expr
      .replace(/\s+/g, "")
      .split(/([+\-*/()])/)
      .filter(Boolean);
    let pos = 0;

    const parseExpr = (): number => {
      let result = parseTerm();
      while (pos < tokens.length && (tokens[pos] === "+" || tokens[pos] === "-")) {
        const op = tokens[pos++];
        const right = parseTerm();
        if (op === "+") result += right;
        else result -= right;
      }
      return result;
    };

    const parseTerm = (): number => {
      let result = parseFactor();
      while (pos < tokens.length && (tokens[pos] === "*" || tokens[pos] === "/")) {
        const op = tokens[pos++];
        const right = parseFactor();
        if (op === "*") result *= right;
        else result /= right;
      }
      return result;
    };

    const parseFactor = (): number => {
      if (pos >= tokens.length) throw new Error("Неожиданный конец выражения");
      if (tokens[pos] === "(") {
        pos++;
        const result = parseExpr();
        if (pos >= tokens.length || tokens[pos] !== ")") {
          throw new Error("Ожидалась закрывающая скобка");
        }
        pos++;
        return result;
      }
      const numStr = tokens[pos++];
      const num = Number(numStr);
      if (isNaN(num)) throw new Error(`Некорректное число: ${numStr}`);
      return num;
    };

    return parseExpr();
  }
}
