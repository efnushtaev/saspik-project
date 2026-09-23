[↑ Saspik-cluster](../../README.md)

# ⧫ Mosquitto — MQTT-брокер

#### Eclipse Mosquitto 2.0: шина сообщений между устройствами, backend и WebSocket-клиентами

Mosquitto — MQTT-брокер кластера.
Собирается из `mqtt/Dockerfile` (образ Mosquitto 2.0 с собственным конфигом).
Порты: `1883` (MQTT) и `9001` (WebSocket).
Аутентификация включена (`allow_anonymous false`), разграничение топиков — через ACL.

### Конфигурация (`mosquitto.conf`)

- Прослушивает порты `1883` (MQTT) и `9001` (WebSocket).
- **Аутентификация включена** (`allow_anonymous false`).
- Файл паролей: `/mosquitto/config/passwordfile`.
- `log_type notice` (без поштучного debug — анти-спам).
- Файл ACL: `/mosquitto/config/mosquitto.acl`.

### Аутентификация

Учётные данные по умолчанию:

| Параметр | Значение |
| :--- | :--- |
| Username | `admin` |
| Password | `password123` |

Файл паролей генерируется `mosquitto_passwd` и вшит в Docker-образ. Добавление пользователя или смена пароля:

```bash
docker exec -it saspik_mosquitto /bin/sh
mosquitto_passwd -b /mosquitto/config/passwordfile username password   # новый пользователь
mosquitto_passwd -b /mosquitto/config/passwordfile admin newpassword   # смена пароля
```

### Правила ACL (`mosquitto.acl`)

Порядок правил:

1. **Default deny** — все неразрешённые топики блокируются.
2. **`clients/%c/#`** — каждый клиент может читать/писать в свой namespace (по Client ID).
3. **`sensor/#`** — publish/subscribe для сенсорных топиков.
4. **`units/#`** — publish/subscribe для топиков команд (управление реле, добавлено 2026-07-27).
5. **Лог-топики**: `device/+/+/log`, `server/+/log`, `rule-engine/+/log` (единый JSON-конверт, 2026-09-03).

Сводная таблица паттернов — в [docs/mqtt-topics](../docs/mqtt-topics.md).

### Разделение топиков по Client ID

Namespace-подход (из-за ограничений ACL при анонимном доступе):

- Client `1` → топики `clients/1/test1`, `clients/1/#`.
- Client `2` → топики `clients/2/test2`, `clients/2/#`.

Альтернативы:

1. **С аутентификацией** — директива `client` в ACL для точных имён топиков.
2. **Проверка на уровне приложения** — валидация clientId в `LocalMqttService`.
3. **Перезапись топиков** — сервер переписывает `test1` → `clients/1/test1`.

### Тестирование

```bash
node client.js                # тестовый клиент (с аутентификацией admin/password123)
node test_auth.js             # проверка аутентификации
node test_auth_clientid2.js   # аутентификация с конкретным Client ID
node test-clientid.js         # проверка ограничений по Client ID
```

### Запуск в Docker

Брокер включён в `docker-compose.yml` как сервис `mosquitto`:

```bash
docker compose build mosquitto
docker compose up -d mosquitto
docker compose logs -f mosquitto
```

### Диагностика

**Брокер не стартует** — проверить синтаксис ACL:

```bash
docker run --rm -it eclipse-mosquitto:2.0 mosquitto -c /mosquitto/config/mosquitto.conf --test
```

**Connection refused:**
- Порт `1883` не заблокирован.
- Не запущен другой экземпляр Mosquitto.
- Контейнер поднят: `docker ps | grep mosquitto`.

**ACL не применяется:**
- Для директив `client` Mosquitto требует аутентификацию.
- При `allow_anonymous true` работают только `pattern`-директивы.
- В логах брокера может быть ошибка парсинга ACL.