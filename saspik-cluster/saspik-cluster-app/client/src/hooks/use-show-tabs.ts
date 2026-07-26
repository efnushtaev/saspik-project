import { useState, useEffect } from 'react';
import { useLocation } from 'react-router-dom';

/**
 * Custom hook to determine if tabs should be shown based on current route
 * @returns Boolean indicating if tabs should be shown
 */
export const useShowTabs = (): boolean => {
  const { pathname } = useLocation();
  const [showTabs, setShowTabs] = useState(pathname !== '/');

  useEffect(() => {
    setShowTabs(pathname !== '/');
  }, [pathname]);

  return showTabs;
};