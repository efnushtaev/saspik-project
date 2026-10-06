/**
 * Относительное время от момента `iso` до текущего момента:
 * "только что", "5 сек назад", "3 мин назад", "2 ч назад", "1 д назад"
 */
export function formatRelativeTime(iso?: string): string | undefined {
  if (!iso) return undefined;
  const ts = new Date(iso).getTime();
  if (Number.isNaN(ts)) return undefined;

  const diffSec = Math.max(0, Math.round((Date.now() - ts) / 1000));

  if (diffSec < 5) return 'только что';
  if (diffSec < 60) return `${diffSec} сек назад`;

  const min = Math.floor(diffSec / 60);
  if (min < 60) return `${min} мин назад`;

  const hours = Math.floor(min / 60);
  if (hours < 24) return `${hours} ч назад`;

  const days = Math.floor(hours / 24);
  return `${days} д назад`;
}
