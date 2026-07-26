import { inject, injectable } from "inversify";

import { TYPES, TEMPORARY_ANY } from "../../types";
import { ILogger } from "../../logger/logger.interface";
import { IConfigService } from "../../config/config.service.interface";
import { IMqttService } from ".";

@injectable()
export class MqttService implements IMqttService {
  constructor(
    @inject(TYPES.Logger) private logger: ILogger,
    @inject(TYPES.ConfigService) private config: IConfigService,
    @inject(TYPES.RightechProxyMqttService)
    private rightechProxyMqttService: IMqttService,
  ) {
    this.logger.log("[MqttService] initialized with Rightech proxy");
  }

  async publish(
    topic: string,
    message: string | Buffer,
    options?: TEMPORARY_ANY,
  ): Promise<void> {
    this.logger.log(`[MqttService] publish to ${topic}: ${message.toString()}`);
    return this.rightechProxyMqttService.publish(topic, message, options);
  }

  async subscribe(
    topic: string,
    callback: (topic: string, message: Buffer) => void,
  ): Promise<void> {
    this.logger.log(`[MqttService] subscribe to ${topic}`);
    return this.rightechProxyMqttService.subscribe(topic, callback);
  }

  async unsubscribe(topic: string): Promise<void> {
    this.logger.log(`[MqttService] unsubscribe from ${topic}`);
    return this.rightechProxyMqttService.unsubscribe(topic);
  }
}
