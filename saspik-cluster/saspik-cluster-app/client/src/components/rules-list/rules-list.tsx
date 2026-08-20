import { createCn } from 'bem-react-classname';

import { useRulesListFetching } from '../../hooks/use-rules-list-fetching';
import { RulesListProps, RuleItem } from './types';

import './styles.css';

const cn = createCn('rules-list');

const topicToString = (topic: string | string[]): string => {
  return Array.isArray(topic) ? topic.join(', ') : topic;
};

const triggerDescription = (rule: RuleItem): string => {
  return topicToString(rule.trigger.topic);
};

export const RulesList = ({ unitId }: RulesListProps) => {
  const { rules, loading, error, setRuleEnabled } = useRulesListFetching(unitId);

  if (loading) {
    return <div className={'rotate-scale-up'} />;
  }

  if (error) {
    return <div className={cn()}>Ошибка загрузки: {error}</div>;
  }

  if (rules.length === 0) {
    return <div className={cn()}>Сценарии не найдены</div>;
  }

  return (
    <div className={cn()}>
      {rules.map(rule => (
        <div key={rule.id} className={cn('card')}>
          <div className={cn('card-content')}>
            <div className={cn('card-top')}>
              <div className={cn('title')}>{rule.name || rule.id}</div>
              <button
                type="button"
                className={cn('toggle', {
                  state: rule.enabled ? 'on' : 'off',
                })}
                onClick={() => setRuleEnabled(rule.id, !rule.enabled)}
              >
                {rule.enabled ? 'Вкл' : 'Выкл'}
              </button>
            </div>
            <div className={cn('trigger')}>
              <span className={cn('trigger-label')}>Топик:</span>{' '}
              <code className={cn('trigger-value')}>
                {triggerDescription(rule)}
              </code>
            </div>
          </div>
        </div>
      ))}
    </div>
  );
};