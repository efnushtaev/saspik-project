import { Response } from "express";

export type ControllerResponseMessage<M> = Promise<
  Response<M, Record<string, unknown>>
>;

export type ParamsDicitionary = Record<string, string>;
