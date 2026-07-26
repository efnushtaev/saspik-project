export interface StoredState {
  value: number | string | boolean | null;
  timestamp: Date;
}

export interface IStateStoreService {
  set(objectId: string, value: unknown, timestamp?: Date): Promise<void>;
  get(objectId: string, field?: string): Promise<StoredState | null>;
  getMany(objectIds: string[]): Promise<Record<string, StoredState | null>>;
}
