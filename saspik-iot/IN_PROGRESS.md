Файл «хода работ» по проекту. Задачи ведутся единым списком: сверху — незавершённые (В работе → На доработку → Нужно взять в работу), ниже — Выполнено. Даты не пишутся.

> Незавершённых задач: 2

## Ход работ

- [Нужно взять в работу] Прошивка esp32-relay-x8 (ESP32 Relay Board x8)
  Статус: план согласован, реализация не начата; ТЗ и промпт — [`plans/esp32-relay-x8.md`](../plans/esp32-relay-x8.md).
  - Новый скетч `saspik-iot-sketches/esp32-relay-x8` на базе esp32-boilerplate (saspik-core), без ESP-NOW: подключение WiFi+MQTT напрямую, `unitId3`, objectId `saspik-awqm001` (actuator+wq+mains), clientId `saspik-a`+objectId.
  - Плата: 8 реле на прямых GPIO — relay1..8 = GPIO32, 33, 25, 26, 27, 14, 12, 13; HIGH = вкл; на старте все LOW («none go HIGH at startup» — инициализация пинов первым шагом в setup); GPIO12 — strapping (MTDI), отметить в README.
  - Топики: команды `device/unitId3/saspik-awqm001/relay1..8` с payload `{"state":"ON"|"OFF"}`; retained state `…/state` = `{"relay1":"ON",…,"relay8":"OFF"}`; логи `…/log`, LWT `…/status`. Подписка одна — `commandTopic + "/+"`, handler принимает только relay1..8 и state.
  - Восстановление реле: boot → OFF → после MQTT connect применяем retained `…/state` (идемпотентно, публикация только при реальном изменении — эхо-петель нет); baseline all-OFF публикуем только если за grace ~3 с после connect retained не пришёл. Авто-ребуты оставить как в boilerplate (MQTT >15 мин / WiFi >5 мин → ребут, состояние вернётся из retained).
  - Портал: кнопка GPIO0/BOOT (портал также откроется при пустом NVS).
  - Состав: `platformio.ini` (env `relay_x8_esp32`, `lib_extra_dirs = ../shared` + `../esp32-boilerplate/lib`, `lib_ldf_mode = deep+`, `-Iinclude`), `include/config.h` (суперсет boilerplate-конфига), `src/RelayBank`, `src/RelayCommandHandler`, `src/RelayX8App`, `src/main.cpp`, README по шаблону ⧫.
  - Верификация: `pio run -e relay_x8_esp32`; прошивка и ручной тест (mosquitto_pub на relay1..8, retained после ребута, логи в telegraf) — вручную пользователем.
  - Документация: запись в README скетчей, новые топики в `saspik-cluster/docs/mqtt-topics.md`, регенерация docs.html. Вне скоупа (follow-up): регистрация unitId3/объекта в Mongo для UI, production-ACL mosquitto, интеграция climate-control с unitId3.

- [Нужно взять в работу] Гарантия исполнения команд: подтверждение и heartbeat
  Статус: не начато — переформулировано из «Надёжности подписки» под бизнес-результат.
  - **Зачем:** сейчас устройство может быть `online` и слать данные, но не исполнять команды: результат `subscribe(TOPIC_COMMAND)` в `connectMQTT()` игнорируется, и при неподтверждённой подписке реле не срабатывают до следующего переподключения. Для владельца это «теплица живая», а свет/полив не включаются — инцидент виден только по последствиям, не по системе.
  - **Устройство:** подтверждение приёма команды — ack с меткой канала и результатом (`device/{unitId}/{objectId}/ack`), периодический heartbeat «подписка жива», логирование и повтор неудачного `subscribe()` при подключении.
  - **Сервер/UI (кластер):** команда считается исполненной только после ack или смены `state`; иначе карточка объекта показывает «не подтверждено» вместо оптимистичного переключения.
  - **Критерий готовности:** сценарий «команда отправлена, но не исполнена» воспроизводится на стенде и виден в UI и логах без выезда на объект.
  - **Вне скоупа:** обход ACL (mosquitto отдаёт SUBACK 0 и при запрете — ловится только сквозной проверкой), production-деплой.

- [Выполнено] Устойчивость шлюза к сбою связи: портал, статус, индикация
  Статус: код готов, сборки и host-тесты успешны; проверка на железе — вручную после перепрошивки.
  - Повод: плата `saspik-sawqm001` после переноса в другую сеть ушла в captive portal и провисела там несколько суток. В портале `GatewayApp::update()` выходит до запуска WiFi/MQTT, поэтому ни данных, ни логов, ни статуса нет, а ребут возвращает в тот же портал. Снаружи устройство выглядит мёртвым, хотя железо и прошивка целы.
  - Выход из портала: `WifiConfig` считает простой (нет подключённых клиентов AP и HTTP-запросов) и через `WIFI_CONFIG_PORTAL_TIMEOUT_MS` (10 мин) делает `ESP.restart()` — повторная попытка STA без физического доступа.
  - Индикация связи: `STATUS_LED_PIN` в `config.h` (в `esp32-dht22-led` — тот же GPIO 2, что канал `led`); мигает, пока MQTT не поднят или активен портал, после подключения пин возвращается каналу через новый `ChannelBank::refreshActuators()`.
  - `saspik-core/MqttManager`: периодический republish retained `online` (`MqttConfig::onlineRepublishMs`, в шлюзе 5 мин) — страховка от залипшего `offline` при потере пакета или пересоздании брокера без volume.
  - `startup` с причиной ребута наконец работает: `MqttManager`/`WifiManager` пишут `mqtt-lost`/`wifi-lost` в `IRebootCauseStore` перед `ESP.restart()` (раньше `setRebootCause` не вызывался ниоткуда), `Logger::publishStartup()` очищает причину после отправки — обычное выключение питания не наследует старую.
  - `GatewayApp` перепубликует retained `state` на каждом переподключении (было: один раз за загрузку).
  - Сборки: `pio run` (`esp32-dht22-led`, RAM 13.9% / Flash 63.8%), `pio run -e gateway_esp32`, `pio test -e native` (29/29).

- [Выполнено] Новое устройство unitId2: шлюз DHT22 + LED (esp32-dht22-led)
  Статус: код готов; `pio run` скетча (RAM 13.9%, Flash 63.7%), `gateway_esp32` и `pio test -e native` (29 тестов) успешны; после фикса статуса нужно перепрошить и проверить UI вручную.
  - Скетч `saspik-iot-sketches/esp32-dht22-led` на базе `esp32-boilerplate` (шаблон `GatewayApp`, ядро `saspik-core`): objectId `saspik-sawqm001`, `unitId2`, каналы `temp`/`humi` (DHT22, GPIO4) и `led` (встроенный GPIO2, active low: LOW = вкл). Устройство — единственный шлюз своего юнита, поэтому wildcard-подписка не дублируется.
  - `GatewayApp` даёт WiFi+MQTT+ESP-NOW-мост: своя команда `device/unitId2/saspik-sawqm001/command`, свои данные `sensor/unitId2/saspik-sawqm001`, retained state/status/LWT, лог; команды узлам — подписка `device/unitId2/+/command` и маршрутизация по `NODE_TABLE` (`saspik-aenm001`, MAC заменить).
  - Исправлен баг `saspik-core/MqttManager`: LWT и online публиковались как `online`/`offline`, а сервер ждёт JSON `{"status":"..."}`; из-за этого UI показывал `offline` при живых данных. Теперь payload JSON — проверено по `strings` в прошивке (`{"status":"online"}` / `{"status":"offline"}`).
  - `NODE_TABLE` в `include/config.h`; `ESP_NOW_CHANNEL=1` должен совпадать с каналом Wi-Fi. Без `.env`/`env_config.h`: дефолты в `config.h`, Wi-Fi/MQTT через captive portal в NVS.
  - `src/app/GatewayApp.*` и `app/examples/ChannelBank.*` скопированы из boilerplate; `Dht22Sensor` — драйвер `saspik-core/drivers/sensor`, подключён через `lib_extra_dirs`; дубль из `esp32-boilerplate/src/app/examples` удалён.
  - DHT22 читается через `Dht22Sensor` (реализует `ISensor`): `ChannelBank` вызывает `read()` дважды за интервал — первый вызов отдаёт температуру, второй влажность; DHT-библиотека кэширует физическое чтение на ~2 с, поэтому `temp`/`humi` уходят в одном JSON. `saspik-core/drivers` — пополняемая библиотека драйверов: новые переиспользуемые драйверы добавляются туда, а не в examples/скетч.
  - Регистрация: строка в `devices.md` и `README` скетчей; объект `saspik-sawqm001-device` в Mongo/UI создаётся вручную (в сид не добавлялся).
  - Ручные сценарии: прошить, заменить MAC `saspik-aenm001` в `NODE_TABLE`, подключить DHT22 (GPIO4) и светодиод (GPIO2), проверить `temp`/`humi`, ON/OFF по команде, retained state после перезагрузки, маршрутизацию команд узлу и логи в telegraf.

- [Выполнено] Конфигурируемые каналы: local-mqtt и boilerplate
  Статус: код готов, сборки и host-тесты успешны; прошивка/ручные сценарии — вручную.
  - `esp32-local-mqtt` переведён на новый объект `saspik-awqm001` (`unitId3`): `CHANNELS[]` в `config.h` (имя, пин, роль, модель), два реле на GPIO 13/26. Команда — `device/.../command` `{"channel":"свет","value":"ON"}`, состояние — retained JSON-карта `{"свет":"ON","полив":"OFF"}`; сенсоры — JSON-карта с произвольными именами. GPIO 25 остаётся кнопкой портала.
  - `esp32-boilerplate`: добавлен `ChannelBank` (сборка из `CHANNELS[]`), каналы `свет`/`полив` на 13/26. Device шлёт ESP-NOW DATA/STATE JSON, шлюз публикует DATA as-is в `sensor/...`, STATE — retained в `device/.../state`; команды маршрутизируются на `device/{unitId}/+/command`. Legacy float-поля и `{"state":"ON"}` сохранены для совместимости.
  - Проверено: `pio test -e native` (29 тестов), `pio run -e device_esp32`, `-e gateway_esp32`, `-e sensor_esp32`; `pio run` local-mqtt.
  - Старые `m003`-объекты и прошивка не менялись (вариант A: новое физическое устройство). ID `saspik-awqm001`/`saspik-aenm001` — принятые дефолты.

- [Выполнено] Закрытие пунктов бэклога и коммит boilerplate
  Статус: boilerplate закоммичен; пост-мортем вынесен отдельной задачей.
  - Закрыты по решению пункты бэклога: «инфраструктура прошивки платы контроллера», «рефакторинг скетчей (фабрика подключения)», «тонкая настройка датчиков» — их закрывает `esp32-boilerplate` (saspik-core, DI, три шаблона ролей) и план Этапа 1.
  - `esp32-boilerplate` закоммичен: библиотека saspik-core, GatewayApp/SensorApp/DeviceApp, примеры периферии, host-тесты (Unity); собирается во всех env (gateway/sensor/device × esp32/s2/s3/c3 + native).
  - README: `esp32-local-mqtt` приведён к фактическому objectId `m002`; boilerplate добавлен в README скетчей; формат классификации в README проекта — `saspik-{роли}{протокол}{питание}{nnn}`.

- [Выполнено] Рефакторинг прошивок
  Статус: выполнено — закрыто `esp32-boilerplate`.
  - Добавить стратегию (фабрику) для метода подключения устройства: через конфиг выбирается система подключения, от неё зависит формат сообщений.
  - Создание инфраструктуры для прошивки платы контроллера.
  - Реализация тонкой настройки датчиков (тюнинг).

- [Выполнено] Разработка промпта для бойлерплейта прошивок

- [Выполнено] Стабилизация загрузки esp32-local-mqtt: WDT-цикл (rst:0x8)
  Статус: устранён, плата грузится чисто (rst:0x1), прошивка записана вручную.
  - **Причина:** повреждённый NVS/LittleFS от обрывов прошивки и запись в LittleFS при активном WiFi вызывали вис в setup и WDT-ресет.
  - **Решения:** кастомная `partitions.csv` 4MB (раздел назван `spiffs`, т.к. Arduino-LittleFS ищет его по имени), `upload_speed=115200`, неблокирующий MQTT-connect (интервал + socket timeout + yield).
  - **Вариант B:** из `shared/device-log` полностью удалён кольцевой файловый лог на LittleFS — осталась только NVS-причина ребута + публикация событий в MQTT-конверт (`publishStartupLog` → `event="startup"`).

- [Выполнено] Мок DHT22 (физический датчик не подключён)
  Статус: сделан, энд-ту-энд подтверждено (конверты device/.../log приходят в Influx measurement logs).
  - `USE_MOCK_SENSOR=true` в `config.h`; `publishSensorData()` при моке генерирует плавно меняющиеся temperature (20–26°C) и humidity (37–53%) на основе `sinf(millis())`, минуя `dht` (читался как nan).
  - Топик/конверт без изменений: `sensor/unitId2/saspik.sa.wm.m002`.

- [Выполнено] Устойчивость ESP32 + единый конвейер логирования
  Статус: сделано, плюс единый JSON-конверт по mosquitto→telegraf→influxdb.
  - Восстановление WiFi в `loop()`, авто-ребут при долгой недоступности MQTT (15 мин) и WiFi (5 мин), публикация диагностики в `TOPIC_DIAG`.
  - Модуль `shared/device-log`: кольцевой лог на LittleFS + NVS-причина ребута (позже сокращён до NVS — см. задачу выше).
  - Единый конверт `{level,src,event,msg,topic,unitId,objectId,uptime,cause}` для device/server/rule-engine, лог-топики `device/+/+/log` и т.д.

- [Выполнено] Пост-мортем логирования esp32-local-mqtt
  Статус: выполнено — код закоммичен (`b1c6872`), сборка `pio run -e esp32dev` успешна (RAM 14%, Flash 62.4%).
  - Удалены недостижимые `publishLogEvent("error", ...)` (reboot ×2, connect-error) и `sendDiagnostics()` + `DIAG_PUBLISH_INTERVAL_MS` (main.cpp, config.h).
  - Вместо них — событие `reconnect` с полями `downSec`/`errors` в success-ветке `connectMQTT()` до сброса `mqttLostSinceMs`; счётчик `mqttConnectErrors` в fail-ветке, сброс после успеха; при первом подключении — прежний `event="connect"`.
  - `prevUptime` сохраняется в NVS (`DeviceLog.savePrevUptime()` перед `wifi-lost`/`mqtt-timeout` рестартами) и добавлен в startup-конверт; после отправки `DeviceLog.clearRebootData()` стирает cause и prevUptime — обычная перезагрузка больше не шлёт ложный `cause=mqtt-timeout`.
  - Кластер под эти правки не дорабатывается: telegraf принимает новые числовые поля автоматически, rule-engine на логи не завязан.
  - Проверка на железе после прошивки: (1) старт после авто-ребута — `startup` с `cause`/`prevUptime`, обычный рестарт — без ложного cause; (2) остановить брокер ~на минуту — `reconnect` с `downSec`/`errors`; (3) команда `{"foo":1}` — игнор без краша.

- [Выполнено] Состояние LED в MQTT: прошивка и retained-публикации
  Статус: выполнено — LED и retained-состояние работают на проде; retained-баг публикаций устранён в `a314468`.
  - Причина: на плате была сборка до `a9f4b43` — команда LED и `status` уже были, а `TOPIC_STATE`/`publishLedState()` ещё нет. После прошивки текущего `esp32-local-mqtt` плата публикует retained `{"state":"ON"|"OFF"}` при подключении и каждой смене; проверено end-to-end (MQTT → telegraf → Influx → UI).
  - Retained-баг закрыт: сенсор и лог-конверты публикуются явной перегрузкой `publish(topic, (const uint8_t*)buf, len, false)`, retained остался только у `state`/`status` (`main.cpp`).
  - Открытые пункты вынесены в задачу «Надёжность подписки»: проверка результата `subscribe()` и опциональный периодический republish `state`.
