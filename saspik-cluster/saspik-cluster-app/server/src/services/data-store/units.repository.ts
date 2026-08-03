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
}
