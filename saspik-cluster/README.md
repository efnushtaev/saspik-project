# SASPIK Cluster

[↑ SASPIK Project](../README.md)

MQTT брокер, rule-engine и серверная часть на Node.js / Express / Inversify / MongoDB.

Контекст проекта: `opencode/agents.md → saspik-cluster`.

## Состав проекта

### Программное обеспечение
Основной код кластера. Монорепозиторий, разворачиваемый через Docker Compose.
[atsap-cluster-app →](saspik-cluster-app/README.md)

| Компонент | Описание |
|---|---|
| `server/` | Express.js бэкенд (микросервисная архитектура на Inversify) |
| `client/` | React SPA фронтенд (собирается в nginx multi-stage) |
| `mqtt/` | MQTT-брокер (собственная реализация) |
| `mqtt-rule-engine/` | Движок сценариев для обработки MQTT-сообщений |
| `nginx/` | Конфигурация обратного прокси |
| `docker-compose.yml` | Оркестрация всех сервисов |
| `.env` | Переменные окружения для продакшна |
| `prompts` | Промпты для AI-агентов |

### docs/

Документация по архитектуре и API кластера, выгруженная из Anytype (коллекция «Кластер»):

| Файл | Описание |
|---|---|
| `API.md` | HTTP API: методы для Units, Objects, Rules, History |
| `UnitsService.md` | Конфигурация юнитов (Unit), JSON Schema DTO |
| `ObjectsService.md` | Конфигурация объектов (сенсоры/устройства), JSON Schema DTO |
| `RulesService.md` | Конфигурация правил и сценариев (Rule Engine) |
| `HistoryService.md` | Сервис мониторинга и истории событий |
| `repo.md` | Ссылка на GitHub-репозиторий |
