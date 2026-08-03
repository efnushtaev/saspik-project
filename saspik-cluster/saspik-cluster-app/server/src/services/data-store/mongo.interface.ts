import { Db } from "mongodb";

export interface Mongo {
  connect(maxRetries?: number, delayMs?: number): Promise<void>;
  getDb(): Db;
}
