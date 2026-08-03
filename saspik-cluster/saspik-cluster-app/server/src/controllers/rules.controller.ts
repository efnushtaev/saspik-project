import { Response, Request } from "express";
import { inject, injectable } from "inversify";

import "reflect-metadata";
import {
  DeleteRuleResponse,
  GetRulesListResponse,
  IRulesController,
  RuleResponse,
} from "./rules.controller.interface";
import { BaseController } from "../common/baseController";
import { ILogger } from "../logger/logger.interface";
import { TYPES } from "../types";
import { RulesControllersRoutesURL, RequestMethod } from "../const";
import { IRulesService } from "../services/rules";
import { RuleDto } from "../dto/rules.dto";

@injectable()
export class RulesController
  extends BaseController
  implements IRulesController
{
  constructor(
    @inject(TYPES.Logger) private loggerService: ILogger,
    @inject(TYPES.RulesService)
    private rulesService: IRulesService,
  ) {
    super(loggerService);
    this.bindRoutes<any>([
      {
        path: RulesControllersRoutesURL.RULES_LIST,
        method: RequestMethod.GET,
        func: this.getRulesList,
      },
      {
        path: RulesControllersRoutesURL.RULES_CREATE,
        method: RequestMethod.POST,
        func: this.upsertRule,
      },
      {
        path: RulesControllersRoutesURL.RULES_UPDATE,
        method: RequestMethod.PATCH,
        func: this.setEnabled,
      },
      {
        path: RulesControllersRoutesURL.RULES_DELETE,
        method: RequestMethod.DELETE,
        func: this.deleteRule,
      },
    ]);
  }

  async getRulesList(_: Request, res: Response) {
    const rules = await this.rulesService.getRules();
    return this.ok<GetRulesListResponse>(res, { rules });
  }

  async upsertRule(
    { body }: Request<Record<string, string>, Record<string, unknown>, RuleDto>,
    res: Response,
  ) {
    try {
      const rule = await this.rulesService.upsertRule(body);
      return this.ok<RuleResponse>(res, { rule });
    } catch (error) {
      return this.send(res, 400, {
        error: error instanceof Error ? error.message : "Unknown error",
      });
    }
  }

  async setEnabled(
    { params, body }: Request<{ id: string }, Record<string, unknown>, { enabled: boolean }>,
    res: Response,
  ) {
    const rule = await this.rulesService.setEnabled(params.id, body.enabled);
    if (!rule) {
      return this.send(res, 404, { error: "Rule not found" });
    }
    return this.ok<RuleResponse>(res, { rule });
  }

  async deleteRule({ params }: Request<{ id: string }>, res: Response) {
    const deleted = await this.rulesService.deleteRule(params.id);
    if (!deleted) {
      return this.send(res, 404, { error: "Rule not found" });
    }
    return this.ok<DeleteRuleResponse>(res, { success: true });
  }
}
