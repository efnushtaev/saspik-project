# IN PROGRESS

Файл «хода работ» по проекту. Заполняется вместе по мере необходимости — новые записи добавляются отдельными блоками с датой (новые сверху).

## Ход работ

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