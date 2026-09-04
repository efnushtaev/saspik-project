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

private:
    void loadRebootCause();
    void saveRebootCause(const char* cause);

    char rebootCause_[64];
};

extern DeviceLogManager DeviceLog;
