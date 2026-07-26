#include "SleepManager.h"

// Статическая RTC-переменная для хранения времени между циклами сна
RTC_DATA_ATTR unsigned long SleepManager::_rtcMillis = 0;

SleepManager::SleepManager()
{
}

bool SleepManager::isWakeupFromSleep() const
{
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
}

void SleepManager::goToSleep()
{
    // Сохраняем текущее время
    _rtcMillis += millis();

    Serial.println("Sleep: сохранение времени в RTC...");
    Serial.print("Sleep: RTC Millis = ");
    Serial.println(_rtcMillis);

    uint64_t sleepTimeUs = SLEEP_SECONDS * 1000000ULL;
    Serial.print("Sleep: ");
    Serial.print(SLEEP_SECONDS);
    Serial.println(" сек");

    // Даем время для вывода логов
    delay(2000);

    Serial.flush();
    delay(100);

    // Настройка пробуждения только по таймеру
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_timer_wakeup(sleepTimeUs);

    delay(50);
    esp_deep_sleep_start();

    // Если deep sleep не сработал — watchdog
    while (1) {
        delay(1000);
    }
}