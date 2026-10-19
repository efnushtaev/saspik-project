// SPDX-License-Identifier: MIT
/*
 * test_topic_builder.cpp — host-тесты TopicBuilder (чистая логика, без железа).
 *
 * Проверяет схему топиков {домен}/{unitId}/{objectId}[/суффикс] — единственное
 * место, где живёт эта схема (см. saspik-cluster/docs/mqtt-topics.md).
 */

#include <string.h>

#include <unity.h>

#include "net/TopicBuilder.h"

void setUp() {}
void tearDown() {}

void test_data_topic() {
    char topic[64];
    TopicBuilder::dataTopic(topic, sizeof(topic), "unitId2", "saspik-senb001");
    TEST_ASSERT_EQUAL_STRING("sensor/unitId2/saspik-senb001", topic);
}

void test_command_topic() {
    char topic[64];
    TopicBuilder::commandTopic(topic, sizeof(topic), "unitId2",
                               "saspik-aenm001");
    TEST_ASSERT_EQUAL_STRING("device/unitId2/saspik-aenm001", topic);
}

void test_command_wildcard_topic() {
    char topic[64];
    TopicBuilder::commandWildcardTopic(topic, sizeof(topic), "unitId2");
    TEST_ASSERT_EQUAL_STRING("device/unitId2/+", topic);
}

void test_log_topic() {
    char topic[64];
    TopicBuilder::logTopic(topic, sizeof(topic), "unitId2", "saspik-senb001");
    TEST_ASSERT_EQUAL_STRING("device/unitId2/saspik-senb001/log", topic);
}

void test_status_topic() {
    char topic[64];
    TopicBuilder::statusTopic(topic, sizeof(topic), "unitId2",
                              "saspik-gwqm001");
    TEST_ASSERT_EQUAL_STRING("device/unitId2/saspik-gwqm001/status", topic);
}

void test_small_buffer_truncates_safely() {
    // snprintf гарантирует '\0' даже в маленьком буфере — обрезка не падает.
    char topic[8];
    TopicBuilder::dataTopic(topic, sizeof(topic), "unitId2", "saspik-senb001");
    TEST_ASSERT_EQUAL_STRING("sensor/", topic);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_data_topic);
    RUN_TEST(test_command_topic);
    RUN_TEST(test_command_wildcard_topic);
    RUN_TEST(test_log_topic);
    RUN_TEST(test_status_topic);
    RUN_TEST(test_small_buffer_truncates_safely);
    return UNITY_END();
}