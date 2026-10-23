#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief DeviceLog: хранение причины последнего ребута в NVS.
 *
 * Энергонезависимый лог причины последнего ребута, используемый для
 * диагностики редких зависаний. События диагностики публикуются в MQTT,
 * а причина ребута сохраняется в NVS перед авто-ребутом и доступна
 * после перезагрузки для отправки в MQTT-конверт.
 */
class DeviceLogManager {
public:
    /**
     * @brief Инициализирует менеджер и загружает причину последнего ребута из NVS.
     * @return всегда true (запись в файловую систему не используется).
     */
    bool begin();

    /**
     * @brief Возвращает последний rebootCause, сохранённый ранее ("" если нет).
     */
    const char* rebootCause() const;

    /**
     * @brief Сохраняет причину последнего ребута в NVS.
     */
    void setRebootCause(const char* cause);

    /**
     * @brief true, если причина последнего ребута ещё не была "прочитана"
     *        (т.е. устройство запустилось после авто-ребута).
     */
    bool hasRebootCause() const;

    /**
     * @brief Сохраняет длительность текущей сессии (сек) в NVS.
     *        Вызывается непосредственно перед авто-ребутом, чтобы после
     *        перезагрузки отправить prevUptime в startup-конверте.
     */
    void savePrevUptime();

    /**
     * @brief Длительность сессии до последнего авто-ребута, сек
     *        (0 — не сохранялась, т.е. питания включались заново).
     */
    uint32_t prevUptime() const;

    /**
     * @brief Стирает причину ребута и prevUptime из NVS.
     *        Вызывается после отправки startup-конверта: иначе обычная
     *        перезагрузка повторно отправляла бы ложный cause из прошлой сессии.
     */
    void clearRebootData();

private:
    void loadRebootCause();
    void saveRebootCause(const char* cause);
    void loadPrevUptime();

    char rebootCause_[64];
    uint32_t prevUptime_ = 0;
};

extern DeviceLogManager DeviceLog;
