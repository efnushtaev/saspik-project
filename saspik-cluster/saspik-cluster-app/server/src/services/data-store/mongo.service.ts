import { Db, MongoClient } from "mongodb";
import { inject, injectable } from "inversify";

import { Mongo } from ".";
import { TYPES } from "../../types";
import { IConfigService } from "../../config/config.service.interface";
import { ILogger } from "../../logger/logger.interface";

@injectable()
export class MongoService implements Mongo {
  private client: MongoClient;
  private db: Db | null = null;

  constructor(
    @inject(TYPES.ConfigService) config: IConfigService,
    @inject(TYPES.Logger) private logger: ILogger,
  ) {
    const url = config.get("MONGODB_URL") || "mongodb://localhost:27017/saspik";
    this.client = new MongoClient(url, { serverSelectionTimeoutMS: 5000 });
  }

  public async connect(maxRetries = 30, delayMs = 2000): Promise<void> {
    for (let attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        await this.client.connect();
        this.db = this.client.db();
        this.logger.log("MongoDB connected successfully");
        return;
      } catch (error) {
        this.logger.error(
          `[MongoService] Attempt ${attempt}/${maxRetries}: MongoDB connection failed -`,
          (error as Error).message,
        );

        if (attempt === maxRetries) {
          this.logger.error("[MongoService] All connection attempts failed");
        }

        await new Promise((resolve) => setTimeout(resolve, delayMs));
      }
    }
  }

  public getDb(): Db {
    if (!this.db) {
      throw new Error("MongoDB is not connected. Call connect() first.");
    }
    return this.db;
  }
}
