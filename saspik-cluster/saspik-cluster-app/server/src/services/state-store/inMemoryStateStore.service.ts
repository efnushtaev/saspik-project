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

  async set(objectId: string, value: unknown, timestamp?: Date): Promise<void> {
    this.store.set(objectId, {
      value: value as number | string | boolean | null,
      timestamp: timestamp ?? new Date(),
    });
  }

  async get(objectId: string): Promise<StoredState | null> {
    return this.store.get(objectId) ?? null;
  }

  async getMany(objectIds: string[]): Promise<Record<string, StoredState | null>> {
    const result: Record<string, StoredState | null> = {};
    for (const id of objectIds) {
      result[id] = this.store.get(id) ?? null;
    }
    return result;
  }
}
