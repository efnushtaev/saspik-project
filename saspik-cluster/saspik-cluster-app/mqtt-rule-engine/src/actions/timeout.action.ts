import { Action } from './action.interface';
import { MessageContext } from '../context';
import { IMqttAdapter } from '../mqtt';

export class TimeoutAction implements Action {
  constructor(
    private delay: number,
    private actions: Action[],
  ) {}

  async execute(ctx: MessageContext, publisher: IMqttAdapter): Promise<void> {
    setTimeout(() => {
      for (const action of this.actions) {
        action.execute(ctx, publisher).catch((err) => {
          console.error(`[TimeoutAction] error in delayed action:`, err);
        });
      }
    }, this.delay);
  }
}
