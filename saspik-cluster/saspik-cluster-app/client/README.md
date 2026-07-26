# Atsap Cluster App — Клиент

## Описание

React SPA для панели управления IoT-кластером. Реализована на CRA + TypeScript, React 19, react-router-dom 6, BEM-стилизация (`bem-react-classname`). Поддерживает мок-режим без бэкенда.

## Функциональность

- Список юнитов с карточками
- Мониторинг сенсоров (температура, влажность, давление и т.д.)
- Автоматика — объекты автоматизации
- 3 таба навигации: Мониторинг, Автоматика, Информация
- «Живые» часы с синхронизацией через `/api/getTimestamp` и локальным тиком каждую секунду
- Адаптивная вёрстка (mobile < 800px / desktop)
- Тёмная тема через CSS-переменные
- Mock-режим (`REACT_APP_MOCK_MODE=true`) — работа без сервера

## Внутренняя структура

```
client/
├── public/
│   ├── index.html              # HTML-точка входа
│   └── manifest.json           # PWA-манифест
├── src/
│   ├── index.tsx               # Точка входа React
│   ├── index.css               # Глобальные стили (тёмная тема)
│   ├── setupTests.ts           # Настройка Jest
│   ├── components/
│   │   ├── app/                # App.tsx, роутинг, переключение mobile/desktop
│   │   ├── top-bar/            # Верхняя панель: часы, заголовок
│   │   ├── control-bar/        # Панель управления: табы, action-кнопки, поиск
│   │   │   ├── tabs/           # 3 таба (Мониторинг, Автоматика, Информация)
│   │   │   └── action-field/   # Кнопка назад, поиск, доп. кнопки
│   │   ├── pages-routes/       # Страницы (Main, Monitoring, Automation, Info)
│   │   ├── units-list/         # Сетка карточек юнитов
│   │   ├── units-card/         # Карточка юнита
│   │   ├── objects-list/       # Сетка карточек сенсоров/автоматики
│   │   ├── objects-card/       # Карточка сенсора/автоматики
│   │   ├── mock-api.ts         # Мок-сервис
│   │   ├── mock-data.ts        # Мок-данные
│   │   └── constants.ts        # URL и пути навигации
│   ├── hooks/
│   │   ├── use-timestamp.ts    # Опрос /api/getTimestamp
│   │   ├── use-mobile-detection.ts
│   │   ├── use-show-tabs.ts
│   │   └── use-objects-list-fetching.ts
│   ├── utils/
│   │   ├── format-date.ts      # Форматирование даты (рус. локаль)
│   │   └── transform-object-to-card.ts
├── build/                      # Сборка CRA
├── ../shared/                  # Общие TypeScript-типы (подключаются через @shared/*)
├── Dockerfile                  # Многостадийная сборка
├── package.json
└── tsconfig.json
```

## Запуск

```bash
npm install
npm start              # dev-режим, бэкенд на :3001
npm run start:mock     # мок-режим, бэкенд не нужен
npm run build          # production-сборка
```

## Переменные окружения

| Переменная | Описание | По умолчанию |
|---|---|---|
| `REACT_APP_MOCK_MODE` | Включить мок-данные | `false` |

## Docker

Многостадийная сборка через `Dockerfile` (node:18-alpine для сборки → nginx alpine для раздачи статики). Используется в `docker-compose.yml` корневого проекта.

---

*Корневой README проекта: [`../README.md`](../README.md)*
