[↑ Назад](../README.md)

# ⧫ Saspik-cluster

#### Микросервисное ядро САСПИК на NodeJS

Центральный узел платформы САСПИК, объединяющий оборудование и процессы в единую управляемую систему.

Кластер собирает данные со всех подключённых устройств, даёт пользователю единую точку управления через веб-интерфейс и связывает элементы сети (Юниты и Объекты) в гибкую иерархию.

Его задача — быть надёжным и независимым ядром: каждый кластер работает сам по себе, но при необходимости может взаимодействовать с другими кластерами, сохраняя целостность системы при любом масштабе.

### Функциональные компоненты

| Компонент | Описание |
|---|---|
| [server](saspik-cluster-app/server/README.md) | API. ExpressJS бэкенд (микросервисная архитектура на Inversify) |
| [client](saspik-cluster-app/client/README.md) | UI, React SPA (CRA + TypeScript), панель управления IoT-кластером |
| [mqtt](saspik-cluster-app/mqtt/README.md) | Eclipse Mosquitto MQTT-брокер с аутентификацией и ACL  |
| [mqtt-rule-engine](saspik-cluster-app/mqtt-rule-engine/README.md) | Движок сценариев автоматизации для обработки данных и управления Объектами |

### Архитектура

- **Frontend**: React SPA, обслуживается через Nginx. Отображает список Юнитов, сенсоры и устройства, вложенные в каждый Юнит. Поддерживает создание/редактирование/удаление Юнитов и Объектов.
- **MongoDB**: Документная БД. Хранит Юниты, Объекты и правила (коллекции `units`, `objects`, `rules`). При первой инициализации заполняется сидами из `server/src/data/*.config.ts` и `server/data/rules.json`.
- **InfluxDB**: Time-series база данных. Хранит все MQTT-сообщения от устройств.
- **Telegraf**: Подписывается на все MQTT-топики (`#`), парсит JSON и пишет в InfluxDB.
- **Backend**: Express.js сервер. Читает Юниты/Объекты/правила из MongoDB, последние значения объектов из InfluxDB и отдаёт их через REST API.
- **Nginx**: Обратный прокси: статика фронтенда, прокси `/api/*` на backend.
- **Mosquitto**: MQTT-брокер для обмена данными с устройствами.
- **MQTT Rule Engine**: Получает правила из MongoDB (или HTTP API/файла), обрабатывает MQTT-сообщения и выполняет правила климат-контроля (температура/влажность).

### Документация
Лежит в `/docs` в корневой папке проекта

| Файл | Описание |
|---|---|
| [API](docs/API.md) | HTTP API: методы для Units, Objects, Rules, History |
| [HistoryService](docs/HistoryService.md) | Сервис мониторинга и истории событий |
| [ObjectsService](docs/ObjectsService.md) | Конфигурация Объектов (сенсоры/устройства), JSON Schema DTO |
| [RulesService](docs/RulesService.md) | Конфигурация правил и сценариев (Rule Engine) |
| [UnitsService](docs/UnitsService.md) | Конфигурация Юнитов (Unit), JSON Schema DTO |
| [mqtt-topics](docs/mqtt-topics.md) | Список MQTT-топиков |

### Конфигурация Nginx

Файл `nginx/nginx.conf` настраивает:
- **Статические файлы** фронтенда из `/usr/share/nginx/html`
- **Прокси** `/api` на backend (`http://backend:3001`)
- **SPA-режим** (`try_files $uri $uri/ /index.html`)
- **Кэширование** статики на 1 год

### Docker Compose


| Сервис | Контейнер | Лимиты |
|---|---|---|
| `backend` | Express.js | 0.5 CPU, 512MB RAM |
| `nginx` | Nginx + React SPA (multi-stage build) | 0.3 CPU, 256MB RAM |
| `mongo` | MongoDB 7 | 0.5 CPU, 512MB RAM |
| `mosquitto` | Mosquitto | 0.2 CPU, 128MB RAM |
| `mqtt-rule-engine` | Rule engine | 0.3 CPU, 128MB RAM |
| `influxdb` | InfluxDB 2.x | 0.3 CPU, 256MB RAM |
| `telegraf` | Telegraf | 0.2 CPU, 128MB RAM |


### Режим моков

Для разработки и тестирования без реального API:

```bash
cd client
npm run start:mock
```

Переменная `REACT_APP_MOCK_MODE=true` включает мок-данные (4 Юнита с вложенными Объектами).

В production-режиме (`docker compose up`) фронтенд собирается внутри образа nginx (multi-stage), сервер использует InfluxDB для значений Объектов.

Юниты, Объекты и правила хранятся в MongoDB: при первом запуске (пустая БД) они заполняются сидами из `server/src/data/*.config.ts` и `server/data/rules.json` (см. `SeedService`).