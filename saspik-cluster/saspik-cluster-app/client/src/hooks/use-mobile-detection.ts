import { useState, useEffect } from 'react';

/**
 * Custom hook to detect mobile devices based on screen width
 * @param breakpoint - The width breakpoint for mobile detection (default: 800px)
 * @returns Boolean indicating if the device is mobile
 */
export const useMobileDetection = (breakpoint: number = 800): boolean => {
  const [isMobile, setIsMobile] = useState(window.innerWidth < breakpoint);

  useEffect(() => {
    const handleResize = () => {
      setIsMobile(window.innerWidth < breakpoint);
    };

    window.addEventListener('resize', handleResize);
    return () => window.removeEventListener('resize', handleResize);
  }, [breakpoint]);

  return isMobile;
};