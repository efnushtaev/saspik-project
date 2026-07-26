import { Condition } from './condition.interface';
import { MessageContext } from '../context';
import { ComparisonOperator } from './types';
import { JSONPATH_CONDITION_REGEX } from './constants';

/**
 * Условие, проверяющее значение из JSON payload по JSONPath и оператору сравнения.
 * Пример выражения: "$.temperature > 30"
 */
export class JsonPathCondition implements Condition {
  private path: string;
  private operator: ComparisonOperator;
  private expectedValue: any;

  /**
   * Создаёт условие из строки выражения.
   * @param expression - строка вида "$.field operator value".
   */
  constructor(expression: string) {
    const match = expression.match(JSONPATH_CONDITION_REGEX);
    if (!match) {
      throw new Error(`Неверный формат JSONPath условия: ${expression}`);
    }

    this.path = `$.${match[1]}`;
    this.operator = match[2] as ComparisonOperator;
    this.expectedValue = this.parseValue(match[3].trim());
  }

  evaluate(ctx: MessageContext): boolean {
    const actual = ctx.getValue(this.path);
    if (actual === undefined) {
      // Поле отсутствует или JSON невалиден
      return false;
    }

    return this.compare(actual, this.expectedValue, this.operator);
  }

  /**
   * Парсит строковое значение в число, если возможно, иначе оставляет строкой.
   */
  private parseValue(str: string): any {
    // Попробуем как число
    const num = Number(str);
    if (!isNaN(num) && str.trim() !== '') {
      return num;
    }
    // Убираем кавычки, если они есть
    if ((str.startsWith('"') && str.endsWith('"')) || (str.startsWith("'") && str.endsWith("'"))) {
      return str.substring(1, str.length - 1);
    }
    // Булево
    if (str === 'true') return true;
    if (str === 'false') return false;
    // Иначе строка
    return str;
  }

  /**
   * Сравнивает два значения с учётом оператора.
   */
  private compare(a: any, b: any, op: ComparisonOperator): boolean {
    switch (op) {
      case '==':
        return a == b; // Нестрогое сравнение для чисел/строк
      case '!=':
        return a != b;
      case '>':
        return a > b;
      case '>=':
        return a >= b;
      case '<':
        return a < b;
      case '<=':
        return a <= b;
      default:
        return false;
    }
  }
}