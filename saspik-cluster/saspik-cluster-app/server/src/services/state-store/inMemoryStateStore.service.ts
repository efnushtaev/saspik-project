import { inject, injectable } from "inversify";

import { ILogger } from "../../logger/logger.interface";
import { TYPES } from "../../types";
import { IStateStoreService, StoredState } from "./stateStore.interface";

@injectable()
export class InMemoryStateStoreService implements IStateStoreService {
  private store = new Map<string, StoredState>();

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
  ) {
    this.logger.log("[InMemoryStateStoreService] initialized");
  }

  async set(topic: string, value: unknown, timestamp?: Date): Promise<void> {
    this.store.set(topic, {
      value: value as number | string | boolean | null,
      timestamp: timestamp ?? new Date(),
    });
  }

  async get(topic: string): Promise<StoredState | null> {
    return this.store.get(topic) ?? null;
  }

  async getMany(topics: string[]): Promise<Record<string, StoredState | null>> {
    const result: Record<string, StoredState | null> = {};
    for (const topic of topics) {
      result[topic] = this.store.get(topic) ?? null;
    }
    return result;
  }
}
