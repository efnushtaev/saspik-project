// SPDX-License-Identifier: MIT
/*
 * TopicBuilder.h — построение всех MQTT-топиков устройства.
 *
 * Единственное место, где живёт схема топиков
 *   {домен}/{unitId}/{objectId}[/суффикс]
 * (см. saspik-cluster/docs/mqtt-topics.md). Выделено в чистый статический
 * класс, чтобы тесты проверяли топики без железа и чтобы нигде в коде
 * не встречалась ручная конкатенация строк.
 */

#pragma once

#include <stddef.h>

/**
 * @brief Статические построители топиков.
 */
class TopicBuilder {
public:
    /** @brief Данные: sensor/{unitId}/{objectId} */
    static void dataTopic(char* out, size_t cap, const char* unitId,
                          const char* objectId);

    /** @brief Команды: device/{unitId}/{objectId} */
    static void commandTopic(char* out, size_t cap, const char* unitId,
                             const char* objectId);

    /** @brief Подписка на команды всех узлов: device/{unitId}/+ */
    static void commandWildcardTopic(char* out, size_t cap,
                                     const char* unitId);

    /** @brief Логи: device/{unitId}/{objectId}/log */
    static void logTopic(char* out, size_t cap, const char* unitId,
                         const char* objectId);

    /** @brief Статус: device/{unitId}/{objectId}/status */
    static void statusTopic(char* out, size_t cap, const char* unitId,
                            const char* objectId);

private:
    // Общий каркас: домен + идентификаторы + опциональный суффикс.
    static void build(char* out, size_t cap, const char* domain,
                      const char* unitId, const char* objectId,
                      const char* suffix);
};