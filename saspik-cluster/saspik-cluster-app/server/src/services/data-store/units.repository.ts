import { Collection } from "mongodb";
import { inject, injectable } from "inversify";

import { Mongo } from ".";
import { TYPES } from "../../types";
import { UnitEntity } from "./types";

@injectable()
export class UnitsRepository {
  private collection: Collection<UnitEntity>;

  constructor(@inject(TYPES.Mongo) private mongo: Mongo) {
    this.collection = mongo.getDb().collection<UnitEntity>("units");
  }

  async findAll(): Promise<UnitEntity[]> {
    return this.collection.find({}).toArray();
  }

  async findById(id: string): Promise<UnitEntity | null> {
    return this.collection.findOne({ id });
  }

  async create(entity: UnitEntity): Promise<UnitEntity> {
    const existing = await this.collection.findOne({ id: entity.id });
    if (existing) {
      throw new Error(`Unit "${entity.id}" already exists`);
    }
    await this.collection.insertOne(entity);
    const saved = await this.collection.findOne({ id: entity.id });
    if (!saved) {
      throw new Error(`Failed to create unit ${entity.id}`);
    }
    return saved;
  }

  async update(id: string, patch: Partial<UnitEntity>): Promise<UnitEntity | null> {
    await this.collection.updateOne({ id }, { $set: patch });
    return this.collection.findOne({ id });
  }

  async delete(id: string): Promise<boolean> {
    const res = await this.collection.deleteOne({ id });
    return res.deletedCount > 0;
  }
}
