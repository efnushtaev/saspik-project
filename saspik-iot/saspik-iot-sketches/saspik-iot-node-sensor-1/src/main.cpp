// ----------------------------------------------------------------------------
// --- ESP32 NODE — Оркестратор (SRP)
// ----------------------------------------------------------------------------
// Главный файл отвечает только за связывание компонентов и цикл работы.
// Вся логика вынесена в специализированные классы:
//   - sensors/   — датчики (DHT-22, Float sensor)
//   - communication/ — ESP-NOW транспорт
//   - system/    — управление сном
//   - utils/     — фильтрация и форматирование
// ----------------------------------------------------------------------------

#include <Arduino.h>
#include "config.h"
#include "sensors/Dht22Sensor.h"
#include "sensors/FloatSensor.h"
#include "communication/EspNowTransport.h"
#include "system/SleepManager.h"
#include "utils/Formatter.h"

// ----------------------------------------------------------------------------
// Идентификаторы полей для ESP-NOW протокола
// ----------------------------------------------------------------------------
enum : uint8_t {
    FIELD_TEMPERATURE   = 1,
    FIELD_HUMIDITY      = 2,
    FIELD_FLOAT_SENSOR  = 3
};

// ----------------------------------------------------------------------------
// Глобальные компоненты
// ----------------------------------------------------------------------------

static const uint8_t espNowReceiverMac[] = ESP_NOW_RECEIVER_MAC;

Dht22Sensor dhtSensor(DHT22_DATA_D_PIN);
FloatSensor floatSensor(FLOAT_SENSOR_D_PIN);
EspNowTransport espNowTransport(espNowReceiverMac);
SleepManager sleepManager;

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(100);

    if (sleepManager.isWakeupFromSleep())
    {
        Serial.println("Пробуждение из глубокого сна по таймеру...");
        Serial.print("RTC Millis: ");
        Serial.println(sleepManager.getRtcMillis());
    }
    else
    {
        Serial.println("Первый запуск или сброс...");
        sleepManager.resetRtcMillis();
    }

    espNowTransport.begin();
    dhtSensor.begin();
    floatSensor.begin();
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------

void loop()
{
    const unsigned long nowMillis = sleepManager.getRtcMillis() + millis();

    // 1. Читаем датчики
    dhtSensor.read();
    floatSensor.read();

    // 2. Формируем поля для ESP-NOW
    float temperature = dhtSensor.getTemperature();
    float humidity = dhtSensor.getHumidity();
    float floatState = floatSensor.getState();

    EspNowField fields[] = {
        {FIELD_TEMPERATURE,  &temperature, sizeof(temperature)},
        {FIELD_HUMIDITY,     &humidity,    sizeof(humidity)},
        {FIELD_FLOAT_SENSOR, &floatState,  sizeof(floatState)}
    };

    // 3. Отправляем по ESP-NOW
    espNowTransport.send(fields, sizeof(fields) / sizeof(fields[0]), nowMillis);

    // 4. Выводим JSON в Serial
    char jsonBuffer[Formatter::BUFFER_SIZE];
    Formatter::formatSensorData(
        temperature,
        humidity,
        floatState,
        nowMillis,
        0,
        jsonBuffer,
        sizeof(jsonBuffer)
    );
    Serial.println(jsonBuffer);

    // 5. Переход в глубокий сон
    delay(50);
    sleepManager.goToSleep();
}