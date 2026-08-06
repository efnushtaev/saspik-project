[↑ ATSAP Cluster App](../README.md)

# MQTT Rule Engine Worker

Движок правил для MQTT, позволяющий автоматически выполнять действия при получении сообщений в заданные топики с поддержкой условий. Правила загружаются из MongoDB, HTTP API или файла (`rules.json`) и обновляются в рантайме без остановки процесса.

## Архитектура

Проект построен по модульному принципу, каждый компонент находится в своей папке:

- **`src/core/`** – ядро движка (`RuleEngine`), управляет подписками, оценивает условия, выполняет действия.
- **`src/mqtt/`** – адаптер MQTT (`MqttAdapter`), абстракция над библиотекой `mqtt`.
- **`src/conditions/`** – условия (`Condition`): проверка топика по regex, JSONPath, точное совпадение payload, логические композиты (AND, OR, NOT).
- **`src/actions/`** – действия (`Action`): публикация в топик, логирование в консоль.
- **`src/config/`** – парсинг конфигурации (`RuleBuilder`) и файловый источник правил (`ConfigWatcher`).
- **`src/providers/`** – источники правил (`RulesProvider`): `MongoRulesProvider`, `ApiRulesProvider`, фабрика `createRulesProvider`.
- **`src/context/`** – контекст сообщения (`MessageContext`), предоставляет доступ к payload как JSON и извлечение значений по JSONPath.
- **`src/utils/`** – утилиты, например сопоставление топиков с wildcards (`topicMatches`).
- **`test/`** – тестовый клиент, мок-адаптер и примеры правил.

## Источники правил

Источник выбирается переменной окружения `RULES_SOURCE`:

| Значение | Источник | Описание |
|---|---|---|
| `mongo` (по умолчанию) | MongoDB | Коллекция `rules`, поле `enabled` отключает правило |
| `api` | HTTP API сервера | `GET {RULES_API_URL}`, ожидает `{ "rules": [...] }` или `[...]` |
| `file` | Локальный `rules.json` | Файловый источник с отслеживанием изменений (`ConfigWatcher`) |

Все источники поддерживают env-подстановку в правилах (`${VAR}` и `${expr:...}`) и опрос с интервалом `RULES_POLL_INTERVAL_MS` (по умолчанию 5000 мс). При изменении правил движок перестраивает подписки без перезапуска.

Переменные окружения:

```
RULES_SOURCE=mongo            # mongo | api | file
MONGODB_URL=mongodb://mongo:27017/saspik   # для RULES_SOURCE=mongo
RULES_API_URL=http://backend:3001/api/v1/rules  # для RULES_SOURCE=api
RULES_POLL_INTERVAL_MS=5000   # интервал опроса
CONFIG_PATH=./rules.json      # для RULES_SOURCE=file
```

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

## Установка и запуск

### Предварительные требования

- Node.js 18 или выше
- MQTT-брокер (например, Mosquitto)
- MongoDB (для `RULES_SOURCE=mongo`, по умолчанию)

### Установка

```bash
cd mqtt-rule-engine
npm install
```

### Сборка

```bash
npm run build
```

### Запуск основного воркера

По умолчанию правила загружаются из MongoDB (коллекция `rules`). Для локального запуска укажите брокер и источник:

```bash
MQTT_BROKER_URL=mqtt://localhost:1883 \
MONGODB_URL=mongodb://localhost:27017/saspik \
RULES_SOURCE=mongo npm start
```

Для файлового источника создайте `rules.json` в корне проекта (пример ниже) или укажите путь через `CONFIG_PATH`:

```bash
MQTT_BROKER_URL=mqtt://localhost:1883 RULES_SOURCE=file CONFIG_PATH=./rules.json npm start
```

### Запуск тестового клиента

Тестовый клиент использует мок-адаптер и демонстрирует работу движка без реального MQTT-брокера.

```bash
npm run test
```

## Формат конфигурации правил

Конфигурация представляет собой JSON-файл с массивом правил.

```json
{
  "rules": [
    {
      "id": "unique_rule_id",
      "trigger": {
        "topic": "sensor/temperature",
        "qos": 0
      },
      "when": {
        "jsonpath": "$.value > 30"
      },
      "then": [
        {
          "action": "log",
          "params": {
            "level": "warn",
            "message": "Температура превысила порог: {{value}}°C"
          }
        },
        {
          "action": "publish",
          "params": {
            "topic": "alerts/high_temperature",
            "payload": "{\"value\": {{value}}, \"timestamp\": \"{{timestamp}}\"}",
            "qos": 1,
            "retain": false
          }
        }
      ]
    }
  ]
}
```

### Триггер (`trigger`)

- `topic` – строка или массив строк. Может содержать MQTT wildcards `+` (один уровень) и `#` (много уровней).
- `qos` – уровень качества обслуживания (0, 1, 2). По умолчанию 0.

### Условия (`when`)

Условие может быть опущено – тогда правило срабатывает на любое сообщение в топике.

Поддерживаемые типы условий:

| Тип | Параметр | Пример |
|-----|----------|--------|
| `topicRegex` | Регулярное выражение для топика | `"^sensor/.+/status$"` |
| `jsonpath` | Выражение вида `$.field оператор значение` | `"$.temperature > 30"` |
| `payloadEquals` | Точное совпадение payload как строки | `"OK"` |
| `and` | Массив условий (логическое И) | `[{"topicRegex": "^sensor/.+"}, {"jsonpath": "$.value > 0"}]` |
| `or` | Массив условий (логическое ИЛИ) | аналогично `and` |
| `not` | Одно условие (логическое НЕ) | `{"topicRegex": "^test/.+"}` |

### Действия (`then`)

Массив действий, выполняемых последовательно при срабатывании правила.

#### Действие `log`

Логирует сообщение в консоль с заданным уровнем.

Параметры:
- `level` – `info`, `warn`, `error` (по умолчанию `info`)
- `message` – строка, может содержать плейсхолдеры `{{fieldName}}`, которые заменяются на значения из JSON payload.

#### Действие `publish`

Публикует сообщение в указанный топик.

Параметры:
- `topic` – топик для публикации
- `payload` – строка payload, может содержать плейсхолдеры `{{fieldName}}`
- `qos` – QoS публикации (по умолчанию 0)
- `retain` – флаг retain (по умолчанию false)

## Обновление правил в рантайме

Воркер опрашивает источник правил каждые `RULES_POLL_INTERVAL_MS` (по умолчанию 5000 мс) и пересобирает правила при изменении:

- **`mongo` / `api`** — опрос по таймеру; при изменении набора правил движок обновляет подписки без остановки процесса.
- **`file`** — отслеживание изменений файла `rules.json` через `fs.watchFile` (перезагрузка при сохранении).

Правила с `enabled: false` пропускаются и их топики отписываются.

## Добавление новых типов условий и действий

### Новый тип условия

1. Создайте класс, реализующий интерфейс `Condition` (метод `evaluate`).
2. Добавьте константу типа в `src/conditions/constants.ts`.
3. Расширьте метод `buildCondition` в `src/config/builder.ts`, добавив обработку нового типа.

### Новый тип действия

1. Создайте класс, реализующий интерфейс `Action` (метод `execute`).
2. Добавьте константу типа в `src/actions/constants.ts`.
3. Расширьте метод `buildAction` в `src/config/builder.ts`.

## Примеры

Примеры правил находятся в `test/sample-rules.json` и `rules.json`.

## Тестирование

Для модульного тестирования используется мок-адаптер `MockMqttAdapter`, который имитирует подключение к брокеру и позволяет симулировать входящие сообщения.

Запуск тестового клиента:

```bash
npm run test
```

## Структура каталога

```
.
├── Dockerfile
├── example.rules.json
├── package.json
├── README.md
├── rules.json
├── src/
│   ├── index.ts              # Точка входа: MqttAdapter + RuleEngine + провайдер правил
│   ├── core/                 # Ядро движка (RuleEngine)
│   ├── mqtt/                 # MQTT-адаптер
│   ├── conditions/           # Условия
│   ├── actions/              # Действия
│   ├── config/               # Парсинг правил (RuleBuilder), файловый источник (ConfigWatcher)
│   └── providers/            # Источники правил (Mongo, API, фабрика)
├── test/
└── tsconfig.json
```

## Лицензия

MIT