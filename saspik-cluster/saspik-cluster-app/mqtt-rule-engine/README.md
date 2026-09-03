[↑ Saspik-cluster](../../README.md)

# ⧫ MQTT Rule Engine Worker

#### Движок правил для MQTT: автоматическое выполнение действий по условиям

Правила загружаются из MongoDB, HTTP API или файла и обновляются в рантайме без остановки процесса.

### Основные компоненты:
- **Ядро** (`RuleEngine`) — управление подписками, оценка условий, выполнение действий
- **MQTT** (`MqttAdapter`) — абстракция над библиотекой `mqtt`; подключение к брокеру, публикация и подписка
- **Условия** (`Condition`) — проверка топика по regex, JSONPath, точное совпадение payload, логические композиты (AND, OR, NOT), диапазон времени (`timeBetween`)
- **Действия** (`Action`) — публикация в топик, логирование в консоль, отложенное выполнение (`timeout`)
- **Провайдеры правил** (`RulesProvider`) — `MongoRulesProvider`, `ApiRulesProvider`, фабрика `createRulesProvider`
- **Конфигурация** — `RuleBuilder` (парсинг JSON-правил), `ConfigWatcher` (файловый источник с `fs.watch`)
- **Контекст** (`MessageContext`) — доступ к payload как JSON, извлечение значений по JSONPath

### Технические детали:
- **TypeScript**, Node.js 18+
- **MQTT** (библиотека `mqtt`), **MongoDB** (официальный драйвер)
- tslog, ts-node (разработка)

### Внутренняя структура

```
mqtt-rule-engine/
├── src/
│   ├── index.ts                  # Точка входа: MqttAdapter + RuleEngine + провайдер правил
│   ├── core/                     # Ядро движка (RuleEngine)
│   ├── mqtt/                     # MQTT-адаптер (MqttAdapter)
│   ├── conditions/               # Условия (regex, jsonpath, payloadEquals, timeBetween)
│   │   └── composite/            # Составные условия (and, or, not)
│   ├── actions/                  # Действия (publish, log, timeout)
│   ├── config/                   # RuleBuilder (парсинг правил), ConfigWatcher (файловый мониторинг)
│   ├── providers/                # Источники правил (Mongo, API, фабрика)
│   ├── context/                  # Контекст сообщения (MessageContext)
│   └── utils/                    # Утилиты (topic-matcher с wildcards)
├── test/                         # Тестовый клиент, мок-адаптер, примеры правил
├── rules.json                    # Дефолтный файл правил
├── Dockerfile
├── package.json
└── tsconfig.json
```

### Запуск

```bash
npm install
npm run dev             # ts-node (разработка)
npm run build           # компиляция tsc в dist/
npm start               # production: node dist/src/index.js
npm run lint            # ESLint
npm run test            # тестовый клиент на мок-адаптере
npm run clean           # очистка dist/
```

### Переменные окружения

| Переменная | Описание | По умолчанию |
|---|---|---|
| `MQTT_BROKER_URL` | URL MQTT-брокера | `mqtt://localhost:1883` |
| `MQTT_USERNAME` / `MQTT_PASSWORD` | Авторизация на брокере | — |
| `RULES_SOURCE` | Источник правил | `mongo` |
| `MONGODB_URL` | Строка подключения к MongoDB (при `RULES_SOURCE=mongo`) | — |
| `RULES_API_URL` | URL API-сервера (при `RULES_SOURCE=api`) | — |
| `CONFIG_PATH` | Путь к файлу правил (при `RULES_SOURCE=file`) | `./rules.json` |
| `RULES_POLL_INTERVAL_MS` | Интервал опроса источника правил, мс | `5000` |

### Источники правил

| Значение `RULES_SOURCE` | Источник | Описание |
|---|---|---|
| `mongo` | MongoDB | Коллекция `rules`, поле `enabled` отключает правило |
| `api` | HTTP API сервера | `GET {RULES_API_URL}`, ожидает `{ "rules": [...] }` или `[...]` |
| `file` | Локальный `rules.json` | Файловый источник с отслеживанием изменений (`ConfigWatcher`) |

Все источники поддерживают env-подстановку в правилах (`${VAR}` и `${expr:...}`) и опрос с интервалом `RULES_POLL_INTERVAL_MS`. При изменении правил движок перестраивает подписки без перезапуска.

### Формат правила

Правило в MongoDB/API хранится в формате rule-engine. Поле `enabled: false` — «мягкое» отключение: движок пропускает такое правило и не подписывается на его топики.

```json
{
  "id": "temp_emergency_high",
  "trigger": { "topic": "sensor/${CLIMATE_CONTROL_UNIT_ID}/dht22", "qos": 0 },
  "when": { "jsonpath": "$.temperature > 29" },
  "then": [
    { "action": "publish", "params": { "topic": "units/unitId1/commands/a_relay3", "payload": "{\"state\":\"1\"}", "qos": 1 } }
  ],
  "enabled": true
}
```

#### Триггер (`trigger`)
- `topic` — строка или массив строк. Поддерживает MQTT wildcards `+` (один уровень) и `#` (много уровней)
- `qos` — уровень качества обслуживания (0, 1, 2). По умолчанию 0

#### Условия (`when`)

Условие может быть опущено — тогда правило срабатывает на любое сообщение в топике.

| Тип | Параметр | Пример |
|---|---|---|
| `topicRegex` | Регулярное выражение для топика | `"^sensor/.+/status$"` |
| `jsonpath` | Выражение `$.field оператор значение` | `"$.temperature > 30"` |
| `payloadEquals` | Точное совпадение payload как строки | `"OK"` |
| `timeBetween` | Диапазон времени срабатывания | `{ "from": "22:00", "to": "06:00" }` |
| `and` | Массив условий (логическое И) | `[{"topicRegex": "^sensor/.+"}, {"jsonpath": "$.value > 0"}]` |
| `or` | Массив условий (логическое ИЛИ) | аналогично `and` |
| `not` | Одно условие (логическое НЕ) | `{"topicRegex": "^test/.+"}` |

#### Действия (`then`)

Массив действий, выполняемых последовательно при срабатывании правила.

##### `publish` — публикация в топик

| Параметр | Описание | По умолчанию |
|---|---|---|
| `topic` | Топик для публикации | — |
| `payload` | Строка payload, поддерживает плейсхолдеры `{{field}}` | — |
| `qos` | QoS публикации | `0` |
| `retain` | Флаг retain | `false` |

##### `log` — логирование в консоль

| Параметр | Описание | По умолчанию |
|---|---|---|
| `level` | Уровень: `info`, `warn`, `error` | `info` |
| `message` | Строка с плейсхолдерами `{{field}}` | — |

##### `timeout` — отложенное выполнение

| Параметр | Описание |
|---|---|
| `delayMs` | Задержка в миллисекундах |
| `then` | Массив действий для выполнения после задержки |

### Обновление правил в рантайме

Воркер опрашивает источник правил каждые `RULES_POLL_INTERVAL_MS` (по умолчанию 5000 мс) и пересобирает правила при изменении:

- **`mongo` / `api`** — опрос по таймеру; при изменении набора правил движок обновляет подписки без остановки процесса
- **`file`** — отслеживание изменений файла через `fs.watchFile` (перезагрузка при сохранении)

Правила с `enabled: false` пропускаются, их топики отписываются.

### Добавление новых типов условий и действий

**Новый тип условия:**
1. Создайте класс, реализующий интерфейс `Condition` (метод `evaluate`)
2. Добавьте константу типа в `src/conditions/constants.ts`
3. Расширьте метод `buildCondition` в `src/config/builder.ts`

**Новый тип действия:**
1. Создайте класс, реализующий интерфейс `Action` (метод `execute`)
2. Добавьте константу типа в `src/actions/constants.ts`
3. Расширьте метод `buildAction` в `src/config/builder.ts`
