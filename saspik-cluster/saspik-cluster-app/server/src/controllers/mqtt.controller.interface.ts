import { Request, Response } from "express";

export interface IMqttController {
  publish(req: Request, res: Response): Promise<void>;
  subscribe(req: Request, res: Response): Promise<void>;
  unsubscribe(req: Request, res: Response): Promise<void>;
}