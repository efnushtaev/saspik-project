type RightechObjectConfig = {
  auth: Record<string, unknown>;
  x509: Record<string, unknown>;
  levels: Record<string, unknown>;
  position: Record<string, unknown>;
};

type RightechObjectState = Record<
  string,
  string | number | boolean | null | undefined
>;

export type RightechObjectDto = {
  _id: string;
  model: string;
  id: string;
  name: string;
  config: RightechObjectConfig;
  status: string;
  active: boolean;
  links: Record<string, unknown>;
  group: string;
  license: string;
  _storageSize: number;
  state: RightechObjectState;
  processedState?: unknown;
  includeProcessedData?: boolean;
  bot?: Record<string, unknown>;
  description?: string;
};
