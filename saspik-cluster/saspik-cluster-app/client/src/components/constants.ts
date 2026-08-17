export const API_URLS = {
  getTimestamp: '/api/getTimestamp',
  getCount: '/api/getCount',
  count: '/api/count/1',
};

export const NAVIGATION_PATHS: Record<string, string> = {
  sensor: '/monitoring',
  device: '/automation',
};

export const getUnitIdFromPathname = (pathname: string): string => {
  const match = pathname.match(/^\/unit\/([^/]+)/);
  return match ? match[1] : '';
};

export const withUnitPath = (path: string, unitId: string): string => {
  return unitId ? `/unit/${unitId}${path}` : path;
};