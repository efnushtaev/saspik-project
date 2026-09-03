#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief DeviceLog: энергонезависимое кольцевое логирование поверх LittleFS
 *        и хранение причины последнего ребута в NVS.
 *
 * Предназначен для диагностики редких зависаний: события пишутся в файл
 * на LittleFS, а перед авто-ребутом сохраняется причина в NVS. Хвост лога
 * и причина доступны после перезагрузки (например, для отправки в MQTT).
 */
class DeviceLogManager {
public:
    /**
     * @brief Монтирует LittleFS и загружает причину последнего ребута из NVS.
     * @return true при успешном монтировании FS.
     */
    bool begin();

    /**
     * @brief Записывает событие в кольцевой лог (с uptime-штампом).
     * @param fmt Форматная строка (printf-стиль).
     */
    void write(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    /**
     * @brief Возвращает последний rebootCause, сохранённый ранее ("" если нет).
     */
    const char* rebootCause() const;

    /**
     * @brief Сохраняет причину последнего ребута в NVS (store=true сразу пишем).
     */
    void setRebootCause(const char* cause);

    /**
     * @brief Читает последние tailLen байт лога в buf (с NUL-терминатором).
     * @return Количество прочитанных байт (без NUL).
     */
    size_t readTail(char* buf, size_t bufSize);

    /**
     * @brief Очищает кольцевой лог (полезно после того, как причина прочитана).
     */
    void clear();

    /**
     * @brief true, если причина последнего ребута ещё не была "прочитана"
     *        (т.е. устройство запустилось после авто-ребута).
     */
    bool hasRebootCause() const;

private:
    void loadRebootCause();
    void saveRebootCause(const char* cause);

    char rebootCause_[64];
    bool fsMounted_ = false;
};

extern DeviceLogManager DeviceLog;
