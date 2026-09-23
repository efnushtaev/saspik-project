#include "Formatter.h"
#include <time.h>

namespace Formatter {

// ----------------------------------------------------------------------------
// Вспомогательная функция: конвертация unsigned long (millis) в ISO 8601 строку
// ----------------------------------------------------------------------------
static void millisToIso8601(unsigned long millis, unsigned long unixEpochOffset, char* out, size_t outSize) {
    if (unixEpochOffset == 0) {
        snprintf(out, outSize, "%lu", millis);
        return;
    }

    time_t unixTime = static_cast<time_t>(unixEpochOffset + (millis / 1000));
    unsigned int ms = millis % 1000;

    struct tm timeinfo;
    gmtime_r(&unixTime, &timeinfo);

    strftime(out, outSize, "%Y-%m-%dT%H:%M:%S", &timeinfo);
    size_t len = strlen(out);
    if (len + 6 < outSize) {
        snprintf(out + len, outSize - len, ".%03uZ", ms);
    }
}

// ----------------------------------------------------------------------------
// Форматирование данных датчиков в JSON-строку
// ----------------------------------------------------------------------------
char* formatSensorData(
    float temperature,
    float humidity,
    float floatSensor,
    unsigned long timestamp,
    unsigned long unixEpochOffset,
    char* buffer,
    size_t bufferSize
) {
    if (buffer == nullptr || bufferSize == 0) {
        return nullptr;
    }

    char tsBuf[40];
    millisToIso8601(timestamp, unixEpochOffset, tsBuf, sizeof(tsBuf));

    int written = snprintf(buffer, bufferSize,
        "{"
        "\"temperature\": %.1f, "
        "\"humidity\": %.1f, "
        "\"float_sensor\": %.1f, "
        "\"timestamp\": %s"
        "}",
        temperature,
        humidity,
        floatSensor,
        tsBuf
    );

    if (written < 0 || static_cast<size_t>(written) >= bufferSize) {
        buffer[bufferSize - 1] = '\0';
    }

    return buffer;
}

} // namespace Formatter