import { inject, injectable } from "inversify";
import { InfluxDB, QueryApi } from "@influxdata/influxdb-client";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { TYPES } from "../../types";
import { IStateStoreService, StoredState } from "./stateStore.interface";

@injectable()
export class InfluxDbStateStoreService implements IStateStoreService {
  private queryApi: QueryApi;
  private bucket: string;

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
  ) {
    const url = this.config.get("INFLUXDB_URL") || "http://localhost:8086";
    const token = this.config.get("INFLUXDB_TOKEN") || "";
    const org = this.config.get("INFLUXDB_ORG") || "saspik";
    this.bucket = this.config.get("INFLUXDB_BUCKET") || "mqtt";

    const client = new InfluxDB({ url, token });
    this.queryApi = client.getQueryApi(org);

    this.logger.log(
      `[InfluxDbStateStoreService] connected to ${url}, org=${org}, bucket=${this.bucket}`,
    );
  }

  async set(
    _topic: string,
    _value: unknown,
    _timestamp?: Date,
  ): Promise<void> {
    // no-op — Telegraf writes all MQTT messages to InfluxDB
  }

  async get(topic: string, field?: string): Promise<StoredState | null> {
    const flux = `
      from(bucket: "${this.bucket}")
        |> range(start: -24h)
        |> filter(fn: (r) => r.topic == "${topic}")
        |> filter(fn: (r) => r._field == "${field || "state"}")
        |> last()
    `;

    try {
      const rows = await this.queryApi.collectRows(flux);
      if (rows.length) {
        const row = rows[0] as Record<string, unknown>;
        return {
          value: (row._value as string | number | boolean | null) ?? null,
          timestamp: new Date(row._time as string),
        };
      }
    } catch (err) {
      this.logger.error(
        `[InfluxDbStateStoreService] query error for ${topic}:`,
        err,
      );
      return null;
    }

    return null;
  }

  async getMany(
    topics: string[],
  ): Promise<Record<string, StoredState | null>> {
    const result: Record<string, StoredState | null> = {};
    for (const topic of topics) {
      result[topic] = await this.get(topic);
    }
    return result;
  }
}
