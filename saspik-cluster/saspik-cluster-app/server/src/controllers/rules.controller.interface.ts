import { NextFunction, Response, Request } from "express";

import { RuleDto } from "../dto/rules.dto";
import { ControllerResponseMessage } from "../common/controller.types";

export type GetRulesListResponse = { rules: RuleDto[] };
export type RuleResponse = { rule: RuleDto };
export type DeleteRuleResponse = { success: boolean };

export interface IRulesController {
  getRulesList: (
    req: Request<Record<string, string>, Record<string, unknown>, unknown, { unitId?: string }>,
    res: Response,
    next: NextFunction,
  ) => ControllerResponseMessage<GetRulesListResponse>;

  upsertRule: (
    req: Request<Record<string, string>, Record<string, unknown>, RuleDto>,
    res: Response,
    next: NextFunction,
  ) => ControllerResponseMessage<RuleResponse>;

  setEnabled: (
    req: Request<{ id: string }, Record<string, unknown>, { enabled: boolean }>,
    res: Response,
    next: NextFunction,
  ) => ControllerResponseMessage<RuleResponse>;

  deleteRule: (
    req: Request<{ id: string }>,
    res: Response,
    next: NextFunction,
  ) => ControllerResponseMessage<DeleteRuleResponse>;
}
