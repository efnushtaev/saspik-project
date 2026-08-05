# ATSAP Cluster App

[↑ SASPIK Cluster](../README.md)

## Состав

- [**client/**](client/README.md) — React SPA (CRA + TypeScript), панель управления IoT-кластером
- [**server/**](server/README.md) — Express.js бэкенд с Dependency Injection (InversifyJS), REST API для Units/Objects/Rules, хранение в MongoDB
- [**mqtt/**](mqtt/README.md) — Eclipse Mosquitto MQTT-брокер с аутентификацией и ACL
- [**mqtt-rule-engine/**](mqtt-rule-engine/README.md) — Движок правил для MQTT: условия, действия, правила из MongoDB/API/файла
- [**telegraf/**](telegraf/) — Конфигурация Telegraf: подписка на все MQTT-топики, запись в InfluxDB

## Описание

Fullstack-приложение для управления IoT-кластером. Фронтенд на React, бэкенд на Express.js, MQTT-брокер Mosquitto и движок правил для автоматизации климат-контроля. Собирается и разворачивается через Docker Compose, предназначено для деплоя через Portainer на VDS.

## Архитектура

- **Frontend**: React SPA, обслуживается через Nginx. Отображает список юнитов, сенсоры и устройства, вложенные в каждый юнит.
- **MongoDB**: Документная БД. Хранит юниты, объекты и правила (коллекции `units`, `objects`, `rules`). При первой инициализации заполняется сидами из `server/src/data/*.config.ts` и `server/data/rules.json`.
- **InfluxDB**: Time-series база данных. Хранит все MQTT-сообщения от устройств.
- **Telegraf**: Подписывается на все MQTT-топики (`#`), парсит JSON и пишет в InfluxDB.
- **Backend**: Express.js сервер. Читает юниты/объекты/правила из MongoDB, последние значения объектов из InfluxDB и отдаёт их через REST API.
- **Nginx**: Обратный прокси: статика фронтенда, прокси `/api/*` на backend.
- **Mosquitto**: MQTT-брокер для обмена данными с устройствами.
- **MQTT Rule Engine**: Получает правила из MongoDB (или HTTP API/файла), обрабатывает MQTT-сообщения и выполняет правила климат-контроля (температура/влажность).

## Endpoints API

| Метод | Путь | Описание |
|---|---|---|
| `GET` | `/health` | Health check |
| `GET` | `/api/getTimestamp` | Текущее время сервера |
| `GET` | `/api/v1/units/list` | Список юнитов с вложенными объектами и правилами (из MongoDB) |
| `POST` | `/api/v1/objects/list/:type` | Объекты по типу (`sensor`/`device`). `value` из InfluxDB |
| `POST` | `/api/v1/objects/getByIds` | Объекты по IDs |
| `POST` | `/api/v1/objects/command/:deviceId` | Команда устройству |
| `POST` | `/api/v1/objects/getLastSensorsData` | Последние показания сенсоров |
| `POST` | `/api/v1/objects` | Создание объекта: `{ id, name, type, spec, description?, unitId }`, топик формируется сервером по паттерну `` `${type}/${unitId}/${id}` `` |
| `GET` | `/api/v1/rules` | Список правил (формат rule-engine, из MongoDB) |
| `POST` | `/api/v1/rules` | Upsert правила (создание/обновление по `id`) |
| `PATCH` | `/api/v1/rules/:id` | Включить/отключить правило (`{ "enabled": true/false }`) |
| `DELETE` | `/api/v1/rules/:id` | Удалить правило |
| `POST` | `/api/v1/mqtt/publish` | Публикация MQTT-сообщения |
| `POST` | `/api/v1/mqtt/subscribe` | Подписка на MQTT-топик |
| `POST` | `/api/v1/mqtt/unsubscribe` | Отписка от MQTT-топика |

## Конфигурация Nginx

Файл [nginx/nginx.conf](nginx/nginx.conf) настраивает:
- Статические файлы фронтенда из `/usr/share/nginx/html`
- Прокси `/api` на backend (`http://backend:3001`)
- SPA-режим (`try_files $uri $uri/ /index.html`)
- Кэширование статики на 1 год

## Docker Compose

Сервисы:

| Сервис | Контейнер | Лимиты |
|---|---|---|
| `backend` | Express.js | 0.5 CPU, 512MB RAM |
| `nginx` | Nginx + React SPA (multi-stage build) | 0.3 CPU, 256MB RAM |
| `mongo` | MongoDB 7 | 0.5 CPU, 512MB RAM |
| `mosquitto` | Mosquitto | 0.2 CPU, 128MB RAM |
| `mqtt-rule-engine` | Rule engine | 0.3 CPU, 128MB RAM |
| `influxdb` | InfluxDB 2.x | 0.3 CPU, 256MB RAM |
| `telegraf` | Telegraf | 0.2 CPU, 128MB RAM |

## Режим моков

Для разработки и тестирования без реального API:

```bash
cd client
npm run start:mock
```

Переменная `REACT_APP_MOCK_MODE=true` включает мок-данные (4 юнита с вложенными объектами).

В production-режиме (`docker compose up`) фронтенд собирается внутри образа nginx (multi-stage), сервер использует InfluxDB для значений объектов. Юниты, объекты и правила хранятся в MongoDB: при первом запуске (пустая БД) они заполняются сидами из `server/src/data/*.config.ts` и `server/data/rules.json` (см. `SeedService`).

## Структура каталога

```
.
├── client/             # React SPA
├── server/             # Express.js API
├── shared/             # Общие TypeScript-типы (client + server)
├── mqtt/               # Mosquitto broker
├── mqtt-rule-engine/   # Правила MQTT
├── nginx/              # Конфигурация Nginx + Dockerfile (multi-stage со фронтендом)
├── telegraf/           # Конфигурация Telegraf + Dockerfile (MQTT → InfluxDB)
├── docker-compose.yml
├── .env / example.env
├── prompts/
├── README.md
└── TODO.md
```

## Требования

- CPU: 1 ядро, RAM: 1 ГБ, Диск: 5 ГБ (минимум)
- CPU: 2 ядра, RAM: 2 ГБ, Диск: 10 ГБ (рекомендуется)
