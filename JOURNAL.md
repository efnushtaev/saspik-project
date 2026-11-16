
Ты работаешь в монорепозитории САСПИК: /Users/efnushtaev/Documents/saspik-project.
Язык ответов и документации — русский; код, идентификаторы и коммиты — на английском.

ПЕРЕД НАЧАЛОМ
1. Прочитай AGENTS.md, saspik-cluster/IN_PROGRESS.md, saspik-iot/IN_PROGRESS.md, saspik-cluster/docs/mqtt-topics.md, saspik-cluster/docs/RulesService.md.
2. Сверь план с фактическим кодом: server/src/app.ts, server/src/types.ts, server/src/services/*, server/src/controllers/*, client/src/components/*, mqtt-rule-engine/src/*, saspik-iot/saspik-iot-sketches/esp32-boilerplate/*, esp32-local-mqtt/*, shared/wifi-config/*.
3. Если найдёшь расхождение с планом — сообщи и уточни, не меняй контракты самовольно.

ЗАДАЧА
Реализовать модуль saspik-kinetic: обучение умного сервопривода Feetech STS3215 вручную, запись траектории, сохранение в кластер, отображение и воспроизведение движения из UI и из сценариев автоматики.

ЗАФИКСИРОВАННЫЕ РЕШЕНИЯ
- Сервопривод: Feetech STS3215, одна ось.
- ESP32 подключается напрямую к WiFi + MQTT, без ESP-NOW.
- Запись: 60 с, 50 Гц, до 3000 кадров, примерно 6 КБ на движение.
- На устройстве траектория хранится в LittleFS в бинарном формате.
- В кластере — отдельный Docker-сервис kinetic (Node/TS + MQTT + Mongo), не модуль backend и не расширение rule-engine.
- Воспроизведение: сервис kinetic стримит кадры на устройство пакетами по MQTT.
- Обучение включается физической кнопкой и командой из UI.
- Активация движения: вручную из UI и через существующие rules.
- В UI добавляется новая вкладка «Кинетика».

ИДЕНТИФИКАЦИЯ И ЖЕЛЕЗО
- objectId: saspik-awqm002, unitId3 по умолчанию.
- Тип объекта: device; spec: [{"key":"позиция","model":"sts3215","unit":"°","minorPart":0}].
- Servo UART: Serial1, TX 17, RX 16, 1 Мбит/с, ID сервопривода 1 (всё настраивается в config.h).
- Кнопка обучения: GPIO 27, INPUT_PULLUP, активный LOW, антидребезг 30 мс.
- Кнопка captive portal остаётся GPIO 25 и работает только при загрузке.
- Питание сервопривода отдельное 6–12 В, общая земля.
- Перед прошивкой проверить свободные GPIO и наличие Arduino-библиотеки Feetech SCServo для выбранной платы.

АРХИТЕКТУРА
ESP32 STS3215 -> MQTT -> kinetic-сервис -> Mongo.
kinetic-сервис:
- принимает upload от устройства;
- хранит движения в Mongo;
- отдаёт HTTP API для UI;
- по команде play стримит кадры на устройство;
- подписан на kinetic/{unitId}/{objectId}/command, поэтому правило может запускать движение обычным publish-действием.

MQTT-КОНТРАКТ
Новый домен: kinetic/{unitId}/{objectId}/...
ACL: добавить kinetic/#; после правки ACL пересобрать mosquitto (docker compose up -d --build mosquitto).

Устройство -> сервис:
- kinetic/{unit}/{object}/upload/start
  {"uploadId":"upl-...","motionId":"kin-...","name":"Движение 1","axes":1,"sampleHz":50,"dtMs":20,"frameCount":3000,"durationMs":60000,"crc32":"..."}
- kinetic/{unit}/{object}/upload/chunk
  {"uploadId":"upl-...","seq":0,"encoding":"b64","data":"..."}
- kinetic/{unit}/{object}/upload/end
  {"uploadId":"upl-...","seq":11,"crc32":"..."}
- kinetic/{unit}/{object}/session
  {"sessionId":"...","state":"playing|done|stopped|error","seq":42,"position":2048}

Сервис -> устройство:
- kinetic/{unit}/{object}/command
  {"action":"teach.start"}
  {"action":"teach.stop"}
  {"action":"play","motionId":"kin-...","sessionId":"..."}
  {"action":"stop"}
- kinetic/{unit}/{object}/frame
  {"sessionId":"...","seq":0,"startMs":0,"dtMs":20,"axes":1,"positions":[2048,2050,2053]}
- kinetic/{unit}/{object}/upload/ack
  {"uploadId":"upl-...","motionId":"kin-...","status":"stored|retry","missing":[3,4]}

Состояние сервопривода публиковать retained в существующий device/{unit}/{object}/state:
{"позиция":"2048","режим":"idle|teach|playing|error"}
Не публиковать state на каждый кадр — не чаще 2 раз в секунду.

ФОРМАТ ТРАЕКТОРИИ
Бинарный формат KIN1:
magic 4 B "KIN1"; version 1 B; axes 1 B; reserved 2 B; sampleHz 2 B; frameCount 4 B; durationMs 4 B; crc32 4 B; frames = frameCount * axes * 2 B (uint16 LE, позиции 0..4095).
В Mongo хранить BSON BinData + metadata: motionId, name, unitId, objectId, durationMs, sampleHz, axes, frameCount, crc32, sizeBytes, preview (до 200 точек), createdAt, updatedAt.
Индексы: {unitId:1, objectId:1}, {createdAt:-1}.

СЕРВИС kinetic
Создать saspik-cluster/saspik-cluster-app/kinetic/:
  package.json, tsconfig.json, Dockerfile, src/{index.ts,config.ts,mqtt,motions,upload,playback,http}
Компоненты:
- MotionRepository — CRUD Mongo.
- UploadAssembler — сборка чанков, CRC, запись в Mongo, ack; идемпотентность по uploadId; при пропусках вернуть retry и missing.
- PlaybackService — загрузка движения, пакеты по 50 кадров, отправка с опережением 1–2 с, один sessionId, одна активная сессия на объект.
- HttpController — API.
- MqttService — подписки kinetic/+/+/upload/*, kinetic/+/+/session, kinetic/+/+/command; публикация frame и upload/ack.

HTTP API:
GET    /api/v1/motions?unitId=&objectId=
GET    /api/v1/motions/:id
PATCH  /api/v1/motions/:id            {"name":"..."}
DELETE /api/v1/motions/:id
POST   /api/v1/motions/:id/play       {"unitId":"...","objectId":"..."}
POST   /api/v1/kinetic/:unitId/:objectId/teach/start
POST   /api/v1/kinetic/:unitId/:objectId/teach/stop
POST   /api/v1/kinetic/:unitId/:objectId/stop
GET    /health

Docker Compose: добавить сервис kinetic, порт 3002, зависимости mongo и mosquitto.
Nginx:
location /api/v1/motions { proxy_pass http://kinetic:3002/api/v1/motions; }
location /api/v1/kinetic { proxy_pass http://kinetic:3002/api/v1/kinetic; }

Ограничения:
- одна активная сессия на объект;
- повторный play останавливает текущую сессию;
- если устройство не отвечает 3 с — сессия завершается ошибкой;
- если сервис перезапустился посреди upload, устройство повторяет загрузку целиком.

ПРОШИВКА esp32-kinetic
Создать saspik-iot/saspik-iot-sketches/esp32-kinetic/ на базе saspik-core:
  platformio.ini, partitions.csv, include/config.h, src/main.cpp,
  src/KineticApp.h/.cpp, src/ServoBus.h/.cpp, src/MotionRecorder.h/.cpp,
  src/MotionStore.h/.cpp, src/MotionUploader.h/.cpp, src/MotionPlayer.h/.cpp,
  src/TeachButton.h/.cpp, test/
Модули:
- ServoBus — обёртка Feetech SCServo: begin, ping, setTorque, readPosition, writePosition; мок для host-тестов.
- MotionRecorder — чистый модуль: фиксированный dtMs=20, буфер 3000 uint16_t, CRC32, кодирование KIN1.
- MotionStore — LittleFS /motions/<motionId>.kin, список файлов, удаление старых при переполнении.
- MotionUploader — idle -> start -> chunks -> end -> ack; base64-чанки по 512 байт; повтор при reconnect; хранить файл до ack stored.
- MotionPlayer — буфер 2–3 пакета, таймер 20 мс, проверка sessionId, стоп при потере кадров >500 мс.
- TeachButton — короткое нажатие старт/стоп записи, длинное 3 с отмена записи.
- KineticApp — composition root: shared/wifi-config для WiFi, saspik-core MqttManager, Logger, LittleFS, ServoBus.

Изменить saspik-core MqttManager: использовать MQTT_PAYLOAD_MAX для setBufferSize и приёма; в config.h kinetic задать MQTT_PAYLOAD_MAX=2048. Для остальных прошивок поведение не менять.

Безопасность:
- в обучении TorqueEnable=0 и чтение present position;
- после остановки удержание позиции с torque on;
- перед playback плавный довод до первого кадра за 300 мс;
- ограничения POS_MIN/POS_MAX и максимальная скорость;
- при потере MQTT/кадров — стоп, удержание, публикация error;
- при 3 ошибках шины подряд — torque off и error.

UI
Добавить маршрут /kinetic и /unit/:unitId/kinetic, вкладку «Кинетика» в control-bar/tabs.
Компоненты: KineticPage, KineticObjectList, MotionList, MotionPreview, TeachControls.
Hooks: use-kinetic-objects, use-motions-list, use-motion-actions.
Добавить mock-api и mock-data для мок-режима.
Функции:
- список устройств с spec.model === "sts3215";
- статус online/offline, текущая позиция, режим;
- кнопки «Начать обучение», «Остановить», «Отменить»;
- список движений: имя, длительность, дата, превью;
- «Воспроизвести», «Стоп», переименовать, удалить;
- «Скопировать JSON для сценария».
На странице «Автоматика» для servo-объекта не показывать ON/OFF-переключатели: показывать позицию и кнопку «Кинетика».
Не добавлять Redux, React Query и UI-библиотеки; следовать текущему стилю BEM + fetch + hooks.

ИНТЕГРАЦИЯ СО СЦЕНАРИЯМИ
Rule-engine не менять. Движение запускается существующим publish-действием:
{
  "id": "kinetic_greeting",
  "name": "Приветствие при нагреве",
  "unitId": "unitId3",
  "trigger": {"topic":"sensor/unitId3/...","qos":0},
  "when": {"jsonpath":"$.temperature > 25"},
  "then": [{"action":"publish","params":{
    "topic":"kinetic/unitId3/saspik-awqm002/command",
    "payload":"{\"action\":\"play\",\"motionId\":\"kin-...\"}",
    "qos":1,
    "retain":false
  }}],
  "enabled": true
}
UI-кнопка «Воспроизвести» вызывает POST /api/v1/motions/:id/play; сервис публикует ту же команду.

ДОКУМЕНТАЦИЯ
Обновить:
- saspik-cluster/docs/mqtt-topics.md — домен kinetic/, upload/command/frame/session;
- saspik-cluster/docs/API.md — /api/v1/motions и /api/v1/kinetic;
- новый saspik-cluster/docs/KineticService.md;
- saspik-cluster/docs/RulesService.md — пример publish в kinetic;
- saspik-iot/devices.md — saspik-awqm002;
- README скетчей saspik-iot;
- AGENTS.md — новый домен, сервис, ACL, команды сборки;
- перегенерировать docs.html скиллом docs-gen.
Telegraf: кадры и upload в Influx не отправлять. Если нужны логи сервиса — добавить kinetic/+/log только в logs-вход.

ПОРЯДОК РАБОТЫ
1. Контракты: docs, ACL, docker-compose, nginx.
2. Скелет kinetic: Express, Mongo, MQTT, health, CRUD метаданных.
3. Приём upload и хранение motions.
4. Прошивка: ServoBus, MotionRecorder, MotionStore, TeachButton.
5. Upload с устройства, ack/retry.
6. Playback-стриминг и MotionPlayer.
7. UI-вкладка и mock-режим.
8. Правило-пример и документация.
9. Сборки и проверки.

ПРОВЕРКА
Сервис:
cd saspik-cluster/saspik-cluster-app/kinetic && npm install && npm run build && npm run lint && npm test
Кластер:
cd saspik-cluster/saspik-cluster-app/server && npm run build
cd ../client && npm run build && npm test
docker compose build kinetic nginx mosquitto
docker compose up -d
Прошивка:
cd saspik-iot/saspik-iot-sketches/esp32-kinetic && pio run -e kinetic_esp32 && pio test -e native

Ручные сценарии:
1. Запись кнопкой 5–10 с -> файл в LittleFS -> motion в Mongo.
2. Запись из UI -> старт/стоп -> появление в списке.
3. Воспроизведение из UI -> сервопривод повторяет движение.
4. Правило с publish -> автоматический запуск.
5. Обрыв MQTT во время записи -> загрузка после reconnect.
6. Обрыв MQTT во время playback -> остановка и error.
7. Перезагрузка устройства -> локальные движения сохраняются.

ПРАВИЛА РАБОТЫ
- Минимальный дифф, без попутного рефакторинга.
- Не оставлять TODO-комментарии и недописанный код.
- Не ломать существующие контракты MQTT/API/ACL/telegraf без явного решения.
- Не менять hostname telegraf, формат топиков и ACL «по ходу».
- Не коммитить секреты.
- Не добавлять зависимости, если задача решается стандартными средствами.
- Коммиты не делать, пока я явно не попрошу.
- После изменений обязательно прогнать сборку/линт/тесты затронутых компонентов.

Начни с этапа 1. Перед изменениями кратко перечисли файлы, которые планируешь затронуть, и подтверди, что прочитал AGENTS.md и текущие контракты.

---
---


Распиновку брать из документации или ты пришлешь?
Изображение в доккументе нужно расположить вертикально, а не как щас горизонтально

Проверить вопросы которые он задает
Уточнить что платы и схемы могут быть разные, но все не более 50 элементов (примерно)


SASPIK-KINETIC
Хочу реализовать следующу логику. Возможно через создание нового модуля для кластера saspik-kinetic. Посмотри на ползовательский суенарий и предложи варианты того как можно это сделать. Речь идет о созлании моудля которое сможет заниматься работой в сервоприводами в рамках проекат саспик. Если будут вопросы - спрашивай
1. Добавить устройство-сервопривод в UI кластера
2. Переключить устройство в режим "обучение", нажатием кнопки на нем
3. Обучить устроство, например, покрутив сервопривод (с учетом что это умный сервопривод который включает в себя энкодер)
4. Координаты полученые при поворачивании сервопривода записываются внутри устройства
5. После повторного нажатия кнопки - данные движений сохраняются в монгудб кластера
6. В UI кластере обновить сценарий автоматики для устройства, там должно появится сохраненное движение
7. При активации сценария - сервопривод должен повторить движение



# Миграция на SourceCraft

https://chat.deepseek.com/share/8gaseoq62am9zuqcib
 1. Всю суть из ваших `PROJECT_CONTEXT.md` и `DETAILS.md` пекревести в формат AGENTS.md
 2. В корне создаем папку .codeassistant кладем туда `rules` и `skills`



#### `.codeassistant/skills/` — ваши навыки в новом формате

Ваши навыки из `.roo/skills/` (например, `schematic-gen`, `session-log`) нужно перенести сюда.

- **Структура навыка:** Каждый навык — это отдельная папка внутри `.codeassistant/skills/`. Внутри обязателен файл `SKILL.md`.
    
- **Формат `SKILL.md`:** SourceCraft использует YAML-подобный заголовок для метаданных и Markdown для инструкций. Пример для `schematic-gen`:

    ```
    name: schematic-gen
    description: "Генерирует схему подключения для ESP32 на основе описания пинов"
    title: Schematic Generator
    metadata:
      inputs:
        board:
          type: string
          description: "Модель платы (например, esp32dev)"
          required: true
          
	Ты — эксперт по электронике. Сгенерируй принципиальную схему для платы {{ board }}...
    
    ```
    
    Инструкции внутри `SKILL.md` могут быть на русском, как и раньше.


#### `.codeassistant/rules/` — тонкая настройка правил

Если в `AGENTS.md` не хочется складывать всё, можно разбить правила на файлы в `.codeassistant/rules/`.[](https://sourcecraft.dev/portal/docs/ru/code-assistant/operations/chat-rules#1)

- **Зачем:** Удобно версионировать и переиспользовать. Например, `01-general.md` (общие стандарты), `02-nodejs.md` (правила для `saspik-cluster`), `03-platformio.md` (для `saspik-iot`).
    
- **Приоритет:** Правила из `.codeassistant/rules/` загружаются вместе с `AGENTS.md` и дополняют друг друга.

---

Устанавливается ли unitId через captivePortal? -- НЕТ а надо бы
ObjectId жестко привязан к unitId? -- НЕТ и хорошо

ВООБЩЕ НЕ ДОЛЖНО


Для работы

