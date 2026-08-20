import { Collection } from "mongodb";
import { inject, injectable } from "inversify";

import { Mongo } from ".";
import { TYPES } from "../../types";
import { RuleEntity } from "./types";

@injectable()
export class RulesRepository {
  private collection: Collection<RuleEntity>;

  constructor(@inject(TYPES.Mongo) private mongo: Mongo) {
    this.collection = mongo.getDb().collection<RuleEntity>("rules");
  }

  async findAll(): Promise<RuleEntity[]> {
    return this.collection.find({}).toArray();
  }

  async findByUnitId(unitId?: string): Promise<RuleEntity[]> {
    if (!unitId) return this.findAll();
    return this.collection.find({ unitId }).toArray();
  }

  async findById(id: string): Promise<RuleEntity | null> {
    return this.collection.findOne({ id });
  }

  async upsert(rule: RuleEntity): Promise<RuleEntity> {
    await this.collection.updateOne(
      { id: rule.id },
      { $set: { ...rule } },
      { upsert: true },
    );
    const saved = await this.collection.findOne({ id: rule.id });
    if (!saved) {
      throw new Error(`Failed to save rule ${rule.id}`);
    }
    return saved;
  }

  async update(
    id: string,
    patch: Partial<RuleEntity>,
  ): Promise<RuleEntity | null> {
    await this.collection.updateOne({ id }, { $set: patch });
    return this.collection.findOne({ id });
  }

  async delete(id: string): Promise<boolean> {
    const res = await this.collection.deleteOne({ id });
    return res.deletedCount > 0;
  }
}
