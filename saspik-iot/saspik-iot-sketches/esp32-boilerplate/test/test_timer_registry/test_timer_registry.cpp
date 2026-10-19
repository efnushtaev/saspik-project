// SPDX-License-Identifier: MIT
/*
 * test_timer_registry.cpp — host-тесты TimerRegistry.
 *
 * Функция времени инжектируется через конструктор (DI) — тесты подставляют
 * фейковые часы и проверяют периодичность и overflow-safe разницу millis().
 */

#include <stdint.h>

#include <unity.h>

#include "core/TimerRegistry.h"

namespace {
uint32_t g_now = 0;
uint32_t g_calls = 0;

uint32_t fakeClock() { return g_now; }

void onTick(void* arg) {
    ++g_calls;
    (void)arg;
}
}  // namespace

void setUp() {
    g_now = 0;
    g_calls = 0;
}

void tearDown() {}

void test_task_fires_by_interval() {
    TimerRegistry reg(&fakeClock);
    TEST_ASSERT_TRUE(reg.add(1000, onTick, nullptr));

    g_now = 500;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(0, g_calls);  // интервал ещё не истёк

    g_now = 1000;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(1, g_calls);

    g_now = 1500;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(1, g_calls);  // следующий тик — на 2000

    g_now = 2000;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(2, g_calls);
}

void test_overflow_safe() {
    TimerRegistry reg(&fakeClock);
    TEST_ASSERT_TRUE(reg.add(1000, onTick, nullptr));

    // Первый тик сразу (lastRun=0, now=0 → разница 0 ≥ 0? нет: 0-0=0 < 1000).
    g_now = 0;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(0, g_calls);

    g_now = 1000;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(1, g_calls);

    // Переполнение millis(): now=0xFFFFFFFF, lastRun=1000 → разница огромная.
    g_now = 0xFFFFFFFFUL;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(2, g_calls);

    // После переполнения: now=100, lastRun=0xFFFFFFFF → разница 101 (wrap),
    // меньше интервала — тикать рано.
    g_now = 100;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(2, g_calls);

    g_now = 1100;
    reg.update();
    TEST_ASSERT_EQUAL_UINT32(3, g_calls);
}

void test_registry_full_rejects_new_task() {
    TimerRegistry reg(&fakeClock);
    for (size_t i = 0; i < TIMER_TASKS_MAX; ++i) {
        TEST_ASSERT_TRUE(reg.add(1000, onTick, nullptr));
    }
    TEST_ASSERT_FALSE(reg.add(1000, onTick, nullptr));
    TEST_ASSERT_EQUAL_UINT32(TIMER_TASKS_MAX, reg.count());
}

void test_null_callback_rejected() {
    TimerRegistry reg(&fakeClock);
    TEST_ASSERT_FALSE(reg.add(1000, nullptr, nullptr));
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_task_fires_by_interval);
    RUN_TEST(test_overflow_safe);
    RUN_TEST(test_registry_full_rejects_new_task);
    RUN_TEST(test_null_callback_rejected);
    return UNITY_END();
}