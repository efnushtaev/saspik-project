# IN PROGRESS

Файл «хода работ» по проекту. Заполняется вместе по мере необходимости — новые записи добавляются отдельными блоками с датой (новые сверху).

## Ход работ

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