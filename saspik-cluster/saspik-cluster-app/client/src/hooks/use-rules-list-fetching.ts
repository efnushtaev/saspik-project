import { useState, useEffect, useCallback } from 'react';
import { useLocation } from 'react-router-dom';

import { mockApi, isMockMode } from '../components/mock-api';
import { RuleItem } from '../components/rules-list/types';

interface ListResponse {
  rules: RuleItem[];
}

export interface UseRulesListResult {
  rules: RuleItem[];
  loading: boolean;
  error: string | null;
  setRuleEnabled: (ruleId: string, enabled: boolean) => Promise<void>;
}

/**
 * Custom hook to fetch unit rules list
 * @param unitIdProp - Unit ID to filter rules by
 * @returns Object containing rules list, loading state, and error state
 */
export const useRulesListFetching = (
  unitIdProp?: string,
): UseRulesListResult => {
  const [rules, setRules] = useState<RuleItem[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);
  const location = useLocation();

  const resolveUnitId = useCallback((): string | undefined => {
    if (unitIdProp) return unitIdProp;
    return new URLSearchParams(location.search).get('id') || undefined;
  }, [unitIdProp, location.search]);

  useEffect(() => {
    const loadingTimer = setTimeout(() => setLoading(false), 10000);

    const fetchRules = async () => {
      try {
        const unitId = resolveUnitId();

        let data;
        if (isMockMode()) {
          data = await mockApi.getRulesList(unitId);
        } else {
          const query = unitId ? `?unitId=${encodeURIComponent(unitId)}` : '';
          const response = await fetch(`/api/v1/rules${query}`);
          if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
          }
          data = await response.json() as ListResponse;
        }
        setRules(data.rules);
      } catch (err) {
        setError(
          err instanceof Error ? err.message : 'An unknown error occurred',
        );
        console.error('Error fetching rules:', err);
      } finally {
        clearTimeout(loadingTimer);
        setLoading(false);
      }
    };

    fetchRules();
    const interval = setInterval(fetchRules, 5000);
    const handleRulesUpdated = () => {
      fetchRules();
    };
    window.addEventListener('rules-updated', handleRulesUpdated);
    return () => {
      window.removeEventListener('rules-updated', handleRulesUpdated);
      clearInterval(interval);
      clearTimeout(loadingTimer);
    };
  }, [location.search, unitIdProp, resolveUnitId]);

  const setRuleEnabled = async (ruleId: string, enabled: boolean) => {
    try {
      if (isMockMode()) {
        await mockApi.setRuleEnabled(ruleId, enabled);
      } else {
        const response = await fetch(`/api/v1/rules/${ruleId}`, {
          method: 'PATCH',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ enabled }),
        });
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
      }
      setRules(prev =>
        prev.map(rule =>
          rule.id === ruleId ? { ...rule, enabled } : rule,
        ),
      );
    } catch (err) {
      console.error('Error setting rule enabled:', err);
    }
  };

  return { rules, loading, error, setRuleEnabled };
};