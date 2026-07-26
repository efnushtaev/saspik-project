import { useState, useEffect, useRef } from 'react';

import { API_URLS } from '../components/constants';
import { TIMESTAMP_MOCK } from './constants';

export const useTimestamp = () => {
  const [timestamp, setTimestamp] = useState(TIMESTAMP_MOCK);

  // Refs для хранения значений между рендерами
  const counterRef = useRef(11);
  const currentTimestampRef = useRef(new Date(timestamp).getTime());

  useEffect(() => {
    const fetchTimestamp = async () => {
      if (counterRef.current < 10) {
        // Локальное обновление времени
        currentTimestampRef.current += 1000;
        setTimestamp(new Date(currentTimestampRef.current).toISOString());
        counterRef.current++;
      } else {
        // Сброс и запрос к серверу
        counterRef.current = 0;
        try {
          const response = await fetch(API_URLS.getTimestamp);
          const data = await response.json();
          const serverTime = new Date(data.timestamp).getTime();
          currentTimestampRef.current = serverTime;
          setTimestamp(data.timestamp);
        } catch (error) {
          console.error('Error fetching timestamp:', error);
          currentTimestampRef.current += 1000;
        }
      }
    };

    // Запускаем сразу и затем каждую секунду
    fetchTimestamp();
    const interval = setInterval(fetchTimestamp, 1000);
    return () => clearInterval(interval);
  }, []);

  return { timestamp };
};
