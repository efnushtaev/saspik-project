#ifndef FORMATTER_H
#define FORMATTER_H

#include <Arduino.h>

/**
 * @brief Форматирование данных датчиков в JSON (SRP)
 * 
 * Преобразует показания датчиков и временную метку в JSON-строку
 * для вывода в Serial или передачи.
 */
namespace Formatter {

/**
 * @brief Максимальный размер JSON-строки
 */
constexpr size_t BUFFER_SIZE = 512;

/**
 * @brief Форматирование данных датчиков в JSON-строку
 * 
 * Формат: {"temperature": <value>, "humidity": <value>,
 *          "float_sensor": <value>, "timestamp": <iso8601|millis>}
 * 
 * @param temperature Температура (°C)
 * @param humidity Влажность (%)
 * @param floatSensor Состояние поплавкового датчика (0 или 1)
 * @param timestamp Временная метка (millis от запуска)
 * @param unixEpochOffset Unix timestamp (сек) соответствующий millis=0 (0 = millis как есть)
 * @param buffer Выходной буфер
 * @param bufferSize Размер буфера
 * @return Указатель на buffer
 */
char* formatSensorData(
    float temperature,
    float humidity,
    float floatSensor,
    unsigned long timestamp,
    unsigned long unixEpochOffset,
    char* buffer,
    size_t bufferSize
);

} // namespace Formatter

#endif // FORMATTER_H