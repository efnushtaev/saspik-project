import { Collection } from "mongodb";
import { inject, injectable } from "inversify";

import { Mongo } from ".";
import { TYPES } from "../../types";
import { ObjectEntity } from "./types";

@injectable()
export class ObjectsRepository {
  private collection: Collection<ObjectEntity>;

  constructor(@inject(TYPES.Mongo) private mongo: Mongo) {
    this.collection = mongo.getDb().collection<ObjectEntity>("objects");
  }

  async findAll(): Promise<ObjectEntity[]> {
    return this.collection.find({}).toArray();
  }

  async findByUnitId(unitId?: string): Promise<ObjectEntity[]> {
    if (!unitId) return this.findAll();
    return this.collection.find({ unitId }).toArray();
  }

  async findByIds(ids: string[]): Promise<ObjectEntity[]> {
    return this.collection.find({ id: { $in: ids } }).toArray();
  }

  async create(entity: ObjectEntity): Promise<ObjectEntity> {
    const existing = await this.collection.findOne({ id: entity.id, unitId: entity.unitId });
    if (existing) {
      throw new Error(`Object "${entity.id}" already exists in unit "${entity.unitId}"`);
    }
    await this.collection.insertOne(entity);
    const saved = await this.collection.findOne({ id: entity.id, unitId: entity.unitId });
    if (!saved) {
      throw new Error(`Failed to create object ${entity.id}`);
    }
    return saved;
  }

  async update(
    id: string,
    unitId: string,
    patch: Partial<ObjectEntity>,
  ): Promise<ObjectEntity | null> {
    await this.collection.updateOne({ id, unitId }, { $set: patch });
    return this.collection.findOne({ id, unitId });
  }

  async delete(id: string, unitId: string): Promise<boolean> {
    const res = await this.collection.deleteOne({ id, unitId });
    return res.deletedCount > 0;
  }
}
