import { inject, injectable } from "inversify";
import { InfluxDB, QueryApi } from "@influxdata/influxdb-client";

import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { TYPES } from "../../types";
import { IStateStoreService, StoredState } from "./stateStore.interface";
import { ObjectsDto } from "../../dto/objects.dto";

@injectable()
export class InfluxDbStateStoreService implements IStateStoreService {
  private queryApi: QueryApi;
  private bucket: string;
  private objectsCache: ObjectsDto[] = [];

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

  async init(objects: ObjectsDto[]): Promise<void> {
    this.objectsCache = objects;
    this.logger.log(
      `[InfluxDbStateStoreService] cached ${this.objectsCache.length} objects`,
    );
  }

  async set(
    _objectId: string,
    _value: unknown,
    _timestamp?: Date,
  ): Promise<void> {
    // no-op — Telegraf writes all MQTT messages to InfluxDB
  }

  async get(objectId: string, field?: string): Promise<StoredState | null> {
    const object = this.objectsCache.find((o) => o.id === objectId);
    if (!object?.topic) return null;

    const flux = `
      from(bucket: "${this.bucket}")
        |> range(start: -24h)
        |> filter(fn: (r) => r.topic == "${object.topic}")
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
        `[InfluxDbStateStoreService] query error for ${object.topic}:`,
        err,
      );
      return null;
    }

    return null;
  }

  async getMany(
    objectIds: string[],
  ): Promise<Record<string, StoredState | null>> {
    const result: Record<string, StoredState | null> = {};
    for (const id of objectIds) {
      result[id] = await this.get(id);
    }
    return result;
  }
}
