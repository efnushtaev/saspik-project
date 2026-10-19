// SPDX-License-Identifier: MIT
/*
 * TopicBuilder.cpp — реализация построителей топиков.
 *
 * Чистый модуль (только snprintf) — покрывается host-тестами.
 */

#include "net/TopicBuilder.h"

#include <stdio.h>

void TopicBuilder::dataTopic(char* out, size_t cap, const char* unitId,
                             const char* objectId) {
    build(out, cap, "sensor", unitId, objectId, nullptr);
}

void TopicBuilder::commandTopic(char* out, size_t cap, const char* unitId,
                                const char* objectId) {
    build(out, cap, "device", unitId, objectId, nullptr);
}

void TopicBuilder::commandWildcardTopic(char* out, size_t cap,
                                        const char* unitId) {
    // Шлюз подписывается на команды любых узлов своего юнита; objectId
    // конкретного узла мост извлекает из топика сообщения.
    snprintf(out, cap, "device/%s/+", unitId);
}

void TopicBuilder::logTopic(char* out, size_t cap, const char* unitId,
                            const char* objectId) {
    build(out, cap, "device", unitId, objectId, "/log");
}

void TopicBuilder::statusTopic(char* out, size_t cap, const char* unitId,
                               const char* objectId) {
    build(out, cap, "device", unitId, objectId, "/status");
}

void TopicBuilder::build(char* out, size_t cap, const char* domain,
                         const char* unitId, const char* objectId,
                         const char* suffix) {
    // Защита от обрезки: snprintf сам допишет '\0', даже если буфер мал.
    if (suffix != nullptr) {
        snprintf(out, cap, "%s/%s/%s%s", domain, unitId, objectId, suffix);
    } else {
        snprintf(out, cap, "%s/%s/%s", domain, unitId, objectId);
    }
}