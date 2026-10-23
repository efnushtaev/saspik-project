Файл «хода работ» по проекту. Заполняется вместе по мере необходимости — новые записи добавляются отдельными блоками с датой (новые сверху).

## Ход работ

### 2026-10-20
### Прошивка esp32-relay-x8 (ESP32 Relay Board x8)

Статус: план согласован, реализация не начата.

- Новый скетч `saspik-iot-sketches/esp32-relay-x8` на базе esp32-boilerplate (saspik-core), без ESP-NOW: подключение WiFi+MQTT напрямую, `unitId3`, objectId `saspik-awqm001` (actuator+wq+mains), clientId `saspik-a`+objectId.
- Плата: 8 реле на прямых GPIO — relay1..8 = GPIO32, 33, 25, 26, 27, 14, 12, 13; HIGH = вкл; на старте все LOW («none go HIGH at startup» — инициализация пинов первым шагом в setup); GPIO12 — strapping (MTDI), отметить в README.
- Топики: команды `device/unitId3/saspik-awqm001/relay1..8` с payload `{"state":"ON"|"OFF"}`; retained state `…/state` = `{"relay1":"ON",…,"relay8":"OFF"}`; логи `…/log`, LWT `…/status`. Подписка одна — `commandTopic + "/+"`, handler принимает только relay1..8 и state.
- Восстановление реле: boot → OFF → после MQTT connect применяем retained `…/state` (идемпотентно, публикация только при реальном изменении — эхо-петель нет); baseline all-OFF публикуем только если за grace ~3 с после connect retained не пришёл. Авто-ребуты оставить как в boilerplate (MQTT >15 мин / WiFi >5 мин → ребут, состояние вернётся из retained).
- Портал: кнопка GPIO0/BOOT (портал также откроется при пустом NVS).
- Состав: `platformio.ini` (env `relay_x8_esp32`, `lib_extra_dirs = ../shared` + `../esp32-boilerplate/lib`, `lib_ldf_mode = deep+`, `-Iinclude`), `include/config.h` (суперсет boilerplate-конфига), `src/RelayBank`, `src/RelayCommandHandler`, `src/RelayX8App`, `src/main.cpp`, README по шаблону ⧫.
- Верификация: `pio run -e relay_x8_esp32`; прошивка и ручной тест (mosquitto_pub на relay1..8, retained после ребута, логи в telegraf) — вручную пользователем.
- Документация: запись в README скетчей, новые топики в `saspik-cluster/docs/mqtt-topics.md`, регенерация docs.html. Вне скоупа (follow-up): регистрация unitId3/объекта в Mongo для UI, production-ACL mosquitto, интеграция climate-control с unitId3.

### 2026-10-19
### Закрытие пунктов бэклога и коммит boilerplate

Статус: boilerplate закоммичен; выполняется только Этап 1 (пост-mortem).

- Закрыты по решению пункты бэклога: «инфраструктура прошивки платы контроллера», «рефакторинг скетчей (фабрика подключения)», «тонкая настройка датчиков» — их закрывает `esp32-boilerplate` (saspik-core, DI, три шаблона ролей) и план Этапа 1.
- `esp32-boilerplate` закоммичен: библиотека saspik-core, GatewayApp/SensorApp/DeviceApp, примеры периферии, host-тесты (Unity); собирается во всех env (gateway/sensor/device × esp32/s2/s3/c3 + native).
- README: `esp32-local-mqtt` приведён к фактическому objectId `m002`; boilerplate добавлен в README скетчей; формат классификации в README проекта — `saspik-{роли}{протокол}{питание}{nnn}`.
- В работе: Этап 1 — пост-mortem логирования esp32-local-mqtt (см. блок 2026-10-14).

### 2026-10-14
### Пост-mortem логирования esp32-local-mqtt

Статус: план согласован, реализация не начата.

- Согласован план исправления мёртвых событий: удалить недостижимые `publishLogEvent("error", ...)` (reboot ×2, connect-error) и `sendDiagnostics()` + `DIAG_PUBLISH_INTERVAL_MS`; вместо них — событие `reconnect` с полями `downSec`/`errors` при восстановлении после обрыва.
- `prevUptime` (секунды сессии до ребута) сохранять в NVS через `DeviceLog` и добавить в startup-конверт; после отправки очищать cause и prevUptime (иначе обычная перезагрузка повторно шлёт ложный `cause=mqtt-timeout`).
- Код в git уже весь m002 — пересборка (`pio run`) и перешивка вручную.

### 2026-10-06
Разработка промпта для бойлерплейта прошивок

### 2026-09-20 — Рефакторинг прошивок

Статус: в работе — приводим прошивки к единой структуре.

- Добавить стратегию (фабрику) для метода подключения устройства: через конфиг выбирается система подключения, от неё зависит формат сообщений.
- Создание инфраструктуры для прошивки платы контроллера.
- Реализация тонкой настройки датчиков (тюнинг).

### 2026-09-04 — Стабилизация загрузки esp32-local-mqtt: WDT-цикл (rst:0x8)

Статус: устранён, плата грузится чисто (rst:0x1), прошивка записана вручную.

- **Причина:** повреждённый NVS/LittleFS от обрывов прошивки и запись в LittleFS при активном WiFi вызывали вис в setup и WDT-ресет.
- **Решения:** кастомная `partitions.csv` 4MB (раздел назван `spiffs`, т.к. Arduino-LittleFS ищет его по имени), `upload_speed=115200`, неблокирующий MQTT-connect (интервал + socket timeout + yield).
- **Вариант B:** из `shared/device-log` полностью удалён кольцевой файловый лог на LittleFS — осталась только NVS-причина ребута + публикация событий в MQTT-конверт (`publishStartupLog` → `event="startup"`).

### 2026-09-04 — Мок DHT22 (физический датчик не подключён)

Статус: сделан, энд-ту-энд подтверждено (конверты device/.../log приходят в Influx measurement logs).

- `USE_MOCK_SENSOR=true` в `config.h`; `publishSensorData()` при моке генерирует плавно меняющиеся temperature (20–26°C) и humidity (37–53%) на основе `sinf(millis())`, минуя `dht` (читался как nan).
- Топик/конверт без изменений: `sensor/unitId2/saspik.sa.wm.m002`.

### 2026-09-03 — Устойчивость ESP32 + единый конвейер логирования

Статус: сделано, плюс единый JSON-конверт по mosquitto→telegraf→influxdb.

- Восстановление WiFi в `loop()`, авто-ребут при долгой недоступности MQTT (15 мин) и WiFi (5 мин), публикация диагностики в `TOPIC_DIAG`.
- Модуль `shared/device-log`: кольцевой лог на LittleFS + NVS-причина ребута (позже сокращён до NVS — см. блок выше).
- Единый конверт `{level,src,event,msg,topic,unitId,objectId,uptime,cause}` для device/server/rule-engine, лог-топики `device/+/+/log` и т.д.