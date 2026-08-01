export interface StoredState {
  value: number | string | boolean | null;
  timestamp: Date;
}

export interface IStateStoreService {
  set(topic: string, value: unknown, timestamp?: Date): Promise<void>;
  get(topic: string, field?: string): Promise<StoredState | null>;
  getMany(topics: string[]): Promise<Record<string, StoredState | null>>;
}
