import { Condition } from './condition.interface';
import { MessageContext } from '../context';

interface TimeBetweenParams {
  start: number;
  end: number;
  field?: string;
}

export class TimeBetweenCondition implements Condition {
  private start: number;
  private end: number;
  private field: string | null;
  private timezoneOffset: number;

  constructor(params: TimeBetweenParams) {
    this.start = params.start;
    this.end = params.end;
    this.field = params.field || null;
    this.timezoneOffset = parseInt(process.env.MOSCOW_OFFSET_HOUR || '3', 10);
  }

  evaluate(ctx: MessageContext): boolean {
    let hour: number;

    if (this.field) {
      const rawValue = ctx.getValue(this.field);
      if (typeof rawValue !== 'string') {
        return false;
      }
      const date = new Date(rawValue);
      if (isNaN(date.getTime())) {
        return false;
      }
      hour = date.getUTCHours() + this.timezoneOffset;
    } else {
      const date = new Date(ctx.timestamp);
      hour = date.getUTCHours() + this.timezoneOffset;
    }

    hour = ((hour % 24) + 24) % 24;

    if (this.start <= this.end) {
      return hour >= this.start && hour < this.end;
    } else {
      return hour >= this.start || hour < this.end;
    }
  }
}
