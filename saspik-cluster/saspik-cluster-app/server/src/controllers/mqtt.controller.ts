import { Request, Response } from "express";
import { inject, injectable } from "inversify";

import "reflect-metadata";
import { IMqttController } from "./mqtt.controller.interface";
import { BaseController } from "../common/baseController";
import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { LocalMqttService } from "../services/mqtt/localMqtt.service";
import { RequestMethod } from "../const";

interface PublishBody {
  topic: string;
  message: string;
  qos?: number;
  retain?: boolean;
}

interface SubscribeBody {
  topic: string;
}

interface UnsubscribeBody {
  topic: string;
}

@injectable()
export class MqttController extends BaseController implements IMqttController {
  constructor(
    @inject(TYPES.Logger) private loggerService: ILogger,
    @inject(TYPES.LocalMqttService) private mqttService: LocalMqttService,
  ) {
    super(loggerService);
    this.bindRoutes([
      {
        path: "/publish",
        method: RequestMethod.POST,
        func: this.publish,
      },
      {
        path: "/subscribe",
        method: RequestMethod.POST,
        func: this.subscribe,
      },
      {
        path: "/unsubscribe",
        method: RequestMethod.POST,
        func: this.unsubscribe,
      },
    ]);
  }

  async publish(req: Request, res: Response): Promise<void> {
    try {
      const { topic, message, qos = 0, retain = false }: PublishBody = req.body;
      if (!topic || !message) {
        this.send(res, 400, { error: "Missing topic or message" });
        return;
      }
      await this.mqttService.publish(topic, message, { qos, retain });
      this.ok(res, { success: true, topic, message });
    } catch (error) {
      this.loggerService.error("[MqttController] publish error:", error);
      this.send(res, 500, { error: "Failed to publish message" });
    }
  }

  async subscribe(req: Request, res: Response): Promise<void> {
    try {
      const { topic }: SubscribeBody = req.body;
      if (!topic) {
        this.send(res, 400, { error: "Missing topic" });
        return;
      }
      // For now, we just subscribe and store a dummy callback.
      // In a real scenario, you might want to register a callback that forwards messages via WebSocket or SSE.
      await this.mqttService.subscribe(topic, (t, msg) => {
        this.loggerService.log(
          `[MqttController] received message on ${t}: ${msg.toString()}`,
        );
      });
      this.ok(res, { success: true, topic });
    } catch (error) {
      this.loggerService.error("[MqttController] subscribe error:", error);
      this.send(res, 500, { error: "Failed to subscribe" });
    }
  }

  async unsubscribe(req: Request, res: Response): Promise<void> {
    try {
      const { topic }: UnsubscribeBody = req.body;
      if (!topic) {
        this.send(res, 400, { error: "Missing topic" });
        return;
      }
      await this.mqttService.unsubscribe(topic);
      this.ok(res, { success: true, topic });
    } catch (error) {
      this.loggerService.error("[MqttController] unsubscribe error:", error);
      this.send(res, 500, { error: "Failed to unsubscribe" });
    }
  }
}
