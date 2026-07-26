import { inject, injectable } from "inversify";
import mqtt, { MqttClient, IClientOptions } from "mqtt";

import { TYPES, TEMPORARY_ANY } from "../../types";
import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IMqttService } from "./mqtt.interface";

@injectable()
export class LocalMqttService implements IMqttService {
  private client: MqttClient | null = null;
  private connected = false;
  private subscriptions: Map<string, (topic: string, message: Buffer) => void> = new Map();

  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
  ) {
    this.logger.log("[LocalMqttService] initializing");
    this.setupConnection();
  }

  private setupConnection(): void {
    const brokerUrl = this.config.get("MQTT_BROKER_URL") || "mqtt://localhost:1883";
    const options: IClientOptions = {
      clientId: `atsap-server-${Date.now()}`,
      clean: true,
      reconnectPeriod: 5000,
      username: this.config.get("MQTT_USERNAME"),
      password: this.config.get("MQTT_PASSWORD"),
    };

    this.logger.log(`[LocalMqttService] connecting to ${brokerUrl}`);
    this.client = mqtt.connect(brokerUrl, options);

    this.client.on("connect", () => {
      this.connected = true;
      this.logger.log("[LocalMqttService] connected to MQTT broker");
      // Resubscribe to existing topics
      this.subscriptions.forEach((callback, topic) => {
        this.client?.subscribe(topic, (err: Error | null) => {
          if (err) {
            this.logger.error(
              `[LocalMqttService] failed to resubscribe to ${topic}:`,
              err,
            );
          }
        });
      });
    });

    this.client.on("message", (topic: string, message: Buffer) => {
      const callback = this.subscriptions.get(topic);
      if (callback) {
        callback(topic, message);
      }
    });

    this.client.on("error", (err: Error) => {
      this.logger.error("[LocalMqttService] MQTT client error:", err);
    });

    this.client.on("close", () => {
      this.connected = false;
      this.logger.log("[LocalMqttService] disconnected from broker");
    });
  }

  async publish(
    topic: string,
    message: string | Buffer,
    options?: TEMPORARY_ANY,
  ): Promise<void> {
    if (!this.client || !this.connected) {
      throw new Error("MQTT client not connected");
    }
    return new Promise((resolve, reject) => {
      this.client!.publish(topic, message, options || {}, (err?: Error) => {
        if (err) {
          this.logger.error(
            `[LocalMqttService] publish error to ${topic}:`,
            err,
          );
          reject(err);
        } else {
          this.logger.log(
            `[LocalMqttService] published to ${topic}: ${message.toString()}`,
          );
          resolve();
        }
      });
    });
  }

  async subscribe(
    topic: string,
    callback: (topic: string, message: Buffer) => void,
    options?: TEMPORARY_ANY,
  ): Promise<void> {
    if (!this.client || !this.connected) {
      throw new Error("MQTT client not connected");
    }
    return new Promise((resolve, reject) => {
      this.client!.subscribe(topic, options || {}, (err: Error | null) => {
        if (err) {
          this.logger.error(
            `[LocalMqttService] subscribe error to ${topic}:`,
            err,
          );
          reject(err);
        } else {
          this.subscriptions.set(topic, callback);
          this.logger.log(`[LocalMqttService] subscribed to ${topic}`);
          resolve();
        }
      });
    });
  }

  async unsubscribe(topic: string): Promise<void> {
    if (!this.client || !this.connected) {
      throw new Error("MQTT client not connected");
    }
    return new Promise((resolve, reject) => {
      this.client!.unsubscribe(topic, (err?: Error) => {
        if (err) {
          this.logger.error(
            `[LocalMqttService] unsubscribe error from ${topic}:`,
            err,
          );
          reject(err);
        } else {
          this.subscriptions.delete(topic);
          this.logger.log(`[LocalMqttService] unsubscribed from ${topic}`);
          resolve();
        }
      });
    });
  }

  async connectBroker(): Promise<void> {
    // Already connected via constructor
    if (this.connected) return;
    this.setupConnection();
  }

  async disconnect(): Promise<void> {
    if (this.client) {
      return new Promise((resolve) => {
        this.client!.end(false, () => {
          this.connected = false;
          this.logger.log("[LocalMqttService] disconnected");
          resolve();
        });
      });
    }
  }
}