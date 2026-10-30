Файл «хода работ» по проекту. Заполняется вместе по мере необходимости — новые записи добавляются отдельными блоками с датой (новые сверху).

## Ход работ

### 2026-10-29
### Конфигурируемые каналы: server/UI/telegraf

Статус: код готов, сборки server/client успешны; end-to-end на железе — вручную.

- Команды переведены на `device/{unitId}/{objectId}/command` с payload
  `{"channel":"<key>","value":"ON"|"OFF"}`; имя канала больше не кодируется в
  топике. Server проверяет, что `key` есть в `device.spec`, иначе команду не
  публикует (раньше молча подставлял первый spec).
- UI: `sendCommand(..., key)`, mock-api, контролы на карточке устройства по
  каждому spec; для `device/` шлём `ON/OFF`, для legacy `units/` — `1/0`.
  В форме объекта подсказка: ключ канала должен совпадать с `config.h`.
- Telegraf: `json_string_fields = ["*"]` — имена каналов произвольные, поэтому
  нельзя перечислять только `state`.
- Influx: добавлено экранирование строк в Flux-запросах.
- Объекты: `saspik-awqm001` (`unitId3`, local-mqtt) и `saspik-aenm001`
  (`unitId2`, boilerplate); старый `m003` не трогаем.
- Проверено: `npm run build` server и client. `npm run lint` server падает на
  `Object.hasOwn` (старый Node/ESLint), к правкам не относится.

### ACL device/# и состояние LED в UI

Статус: выполнено, проверено end-to-end на проде (185.72.145.19).

- Причина «LED включается, но карточка не сохраняет состояние»: живой mosquitto работал со старым ACL `device/+/+/log`; команда, `/state` и `/status` молча отбрасывались. Новый ACL `device/#` уже был в репозитории (`a9f4b43`), но образ не пересобирался.
- После пересборки образа mosquitto и перепрошивки платы проверено: retained `device/unitId3/saspik.sa.wm.m003/status = {"status":"online"}` и `.../state = {"state":"OFF"}`, telegraf пишет `state` в Influx (`mqtt_consumer`), API отдаёт `spec[0].value`/`updatedAt`, карточка сохраняет ON/OFF.
- Диагностический нюанс: mosquitto отдаёт SUBACK `0` даже на запрещённые топики (фильтрация на доставке), поэтому ACL проверяется по факту прохождения сообщений, а не по коду SUBACK.
- Деплой-грабли: ACL вшит в образ (`mqtt/Dockerfile`); `restart`/`up -d` без `--build` его не обновляют. После правки ACL нужен rebuild `mosquitto` или bind-mount `./mqtt/mosquitto.acl`.

### 2026-10-20
### План доработки кластера: UI-фиксы и выравнивание m002

Статус: план согласован, реализация не начата. Графики истории — в бэклог.

- Этап 0 (быстрые фиксы): единые кнопки «назад» — `navigate(-1)` с фолбэком (круглая кнопка ControlBar, unit-page, object-page; фолбэк `/` или канонический список); фикс битого роута `/object/:objectId` (objects-list всегда строит URL с unit); wildcard-матчинг в LocalMqttService (подписка `device/+/+/status` не срабатывает — статусы online/offline в UI мертвы).
- Этап 3 (топики и m002): updateObject больше не пересчитывает topic при правке (сохранять существующий из Mongo — правка реле/LED через UI не рвёт команды/данные); unitId2.config.ts на эталон m002 (dht22 → `sensor/unitId2/saspik.sa.wm.m002`, LED → id `led`, `device/unitId2/saspik.sa.wm.m002`); telegraf подписка данных `led/#` → `device/+/+`; миграция Mongo (m001 не удалять) — зависит от доступности docker, при недоступности откладывается.
- Верификация: сборки server/client, ручные сценарии (назад с диплинка, правка объекта без смены топика, online/offline статусы).
- Вне плана: графики истории (бэклог TODO, разведка API/UI готова), логи в UI (открытая задача IN_PROGRESS).

### 2026-10-14
### Анализ логов unitId2 и план выравнивания на m002

Статус: план согласован, реализация не начата.

- Расшифрованы логи unitId2 за 7 дней (бакет `logs`): один инцидент — недоступность MQTT >15 мин → ребут `cause=mqtt-timeout` → восстановление 10.10 ~19:00 UTC, дальше стабильно. `_time` всех строк — граница часового окна `aggregateWindow`, не точное время событий.
- Выявлены две проблемы: (1) рассинхрон прошивки — `objectId=saspik.sa.wm.m001` при топике `device/unitId2/saspik.sa.wm.m002/log` (на устройстве старая сборка, git-`config.h` уже весь m002); (2) мёртвые ветки логирования — `reboot`/`connect-error`/`diag` не доходят до MQTT из-за guard `!mqttClient.connected()`, в Influx попадают только `connect` и `startup`.
- Согласован план: эталон m002; сервер — `unitId2.config.ts` (sensor → `sensor/unitId2/saspik.sa.wm.m002`, led → `device/unitId2/saspik.sa.wm.m002` c id `led`), telegraf — подписка `device/+/+` для чтения состояния LED, миграция Mongo (m001-объект не удалять).
- Открытые риски: `updateObject()` пересчитывает topic по `type/unitId/id` — правка LED-объекта через UI оборвёт топик (как у реле unitId1).

### 2026-09-28
### Доработка кнопок возврата на предыдущие страницы (frontend)

- Нужна доработка фронта в части кнопок, которые делают возврат на предыдущие страницы.

### 2026-09-20 — Рефакторинг документации

Статус: в работе — доработка и переименование.

- Переименование atsap → saspik (пакеты, нейминг).
- README для nginx/ (client/ — готово).
- Раскрыть разделы docs по конвейеру mosquitto→telegraf→influxdb.

### 2026-09-20 — «Статика» значений датчиков в UI

Статус: устранена, UI показывает живые значения m001.

- **Причина:** telegraf при каждой пересборке стека менял `host` (short container ID) и плодил новые серии в InfluxDB — накопилось 5 серий на `sensor/unitId2/saspik.sa.wm.m001/temperature`; серверный Flux `last()` без `group()` возвращал по таблице на серию и `rows[0]` брал мёртвую серию.
- **Фиксы:** стабильный host через `[agent] hostname = "telegraf"` (топ-уровневая `hostname` в telegraf 1.28 отвергалась парсером → telegraf уходил в restart-loop; перенос в таблицу `[agent]` решает) + `|> group()` перед `sort/limit` в запросе сервера. Коммиты 8948847, c57169a, 9a4f872.
- **Статус m002:** объекты «Дача»/«DHT22» (id `saspik.sa.wm.m002`) не пишут в Influx — их строки в UI пустые по задумке.
- Открытая задача: отображение логов (бакет `logs`) в UI.

### 2026-09-05 — Остановка записи в Influx из-за healthcheck mosquitto

Статус: устранено, свежие точки снова пишутся.

- **Причина:** healthcheck mosquitto каждые 30 с публиковал в `healthcheck/ping` payload `test` (не JSON), а telegraf подписан на `healthcheck/#` с `data_format=json` → ошибка парсинга в mqtt_consumer → падал весь батч.
- **Фикс:** payload заменён на валидный JSON `{"payload":"test"}`, `healthcheck/#` убран из JSON-топиков telegraf.

### 2026-09-03 — Единое логирование по конвейеру + починенные healthcheck'и

Статус: сделано, весь стек healthy.

- Единый JSON-конверт `{level,src,event,msg,topic,unitId,objectId,uptime,cause}` для device/server/rule-engine; лог-топики `device/+/+/log`, `server/+/log`, `rule-engine/+/log`; отдельный measurement `logs` (бакет `logs`, retention 7 дней).
- Исправлены три healthcheck'и в docker-compose: mosquitto (учётки при `allow_anonymous=false`), server (curl → wget в node:18-alpine), nginx (localhost IPv6 → 127.0.0.1).
- Коммиты 268dd72 (unified logging), 7cebe4c (healthcheck fixes).