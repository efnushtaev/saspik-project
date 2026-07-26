/**
 * Утилиты для сопоставления MQTT-топиков с шаблонами (wildcards + и #).
 */

/**
 * Проверяет, соответствует ли конкретный топик MQTT-шаблону.
 * Поддерживает wildcards:
 *   + - один уровень (любая строка)
 *   # - много уровней (ноль или более)
 * @param pattern - шаблон (например, "home/+/temperature").
 * @param topic - конкретный топик (например, "home/room1/temperature").
 * @returns true, если топик соответствует шаблону.
 */
export function topicMatches(pattern: string, topic: string): boolean {
  // Разбиваем на уровни по '/'
  const patternParts = pattern.split('/');
  const topicParts = topic.split('/');

  let pIdx = 0;
  let tIdx = 0;

  while (pIdx < patternParts.length && tIdx < topicParts.length) {
    const p = patternParts[pIdx];
    const t = topicParts[tIdx];

    if (p === '#') {
      // Многоуровневый wildcard – соответствует оставшейся части топика
      return true;
    }

    if (p === '+') {
      // Одноуровневый wildcard – пропускаем текущий уровень
      pIdx++;
      tIdx++;
      continue;
    }

    if (p !== t) {
      return false;
    }

    pIdx++;
    tIdx++;
  }

  // Если остались уровни в шаблоне, они должны быть '#' (с возможным пустым уровнем)
  while (pIdx < patternParts.length) {
    if (patternParts[pIdx] === '#') {
      // '#' может соответствовать нулю уровней
      pIdx++;
    } else {
      break;
    }
  }

  // Если остались уровни в топике, но не в шаблоне – не совпадает, кроме случая, когда в шаблоне остался '#'
  if (tIdx < topicParts.length) {
    return false;
  }

  // Если оба индекса достигли конца – полное совпадение
  return pIdx === patternParts.length && tIdx === topicParts.length;
}

/**
 * Преобразует MQTT-шаблон в регулярное выражение (для внутреннего использования).
 * @param pattern - шаблон с wildcards.
 * @returns RegExp, который можно использовать для тестирования топиков.
 */
export function patternToRegex(pattern: string): RegExp {
  // Экранируем специальные символы regex, кроме + и #
  const escaped = pattern
    .replace(/[\\^$.*+?()[\]{}|]/g, '\\$&')
    .replace(/\+/g, '([^/]+)')
    .replace(/#/g, '(.+)');
  return new RegExp(`^${escaped}$`);
}