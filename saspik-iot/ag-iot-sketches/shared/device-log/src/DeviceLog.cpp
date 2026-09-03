#include "DeviceLog.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <stdarg.h>
#include <stdio.h>

DeviceLogManager DeviceLog;

// ---------------------------------------------------------------------------
// Конфигурация
// ---------------------------------------------------------------------------

// Имя файла кольцевого лога на LittleFS
static const char LOG_FILE[]       = "/log.txt";
// Максимальный размер файла лога. Кольцевая логика: при превышении
// оставляем только последние MAX_LOG_SIZE байт.
static const size_t LOG_MAX_SIZE   = 16 * 1024;
// Буфер для отдельной строки
static const size_t LINE_BUF_SIZE  = 128;

// NVS-пространство и ключи
static const char NVS_NAMESPACE[]  = "devicelog";
static const char KEY_REBOOT_CAUSE[] = "reboot_cause";

// ---------------------------------------------------------------------------
// NVS: причина последнего ребута
// ---------------------------------------------------------------------------

void DeviceLogManager::loadRebootCause() {
    rebootCause_[0] = '\0';
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        prefs.end();
        return;
    }
    String cause = prefs.getString(KEY_REBOOT_CAUSE, "");
    prefs.end();
    cause.toCharArray(rebootCause_, sizeof(rebootCause_));
}

void DeviceLogManager::saveRebootCause(const char* cause) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return;
    }
    if (cause && cause[0] != '\0') {
        prefs.putString(KEY_REBOOT_CAUSE, cause);
    } else {
        prefs.remove(KEY_REBOOT_CAUSE);
    }
    prefs.end();
}

bool DeviceLogManager::begin() {
    fsMounted_ = LittleFS.begin();
    if (!fsMounted_) {
        Serial.println("[device-log] LittleFS mount failed");
    }
    loadRebootCause();
    return fsMounted_;
}

void DeviceLogManager::setRebootCause(const char* cause) {
    snprintf(rebootCause_, sizeof(rebootCause_), "%s", cause ? cause : "");
    saveRebootCause(rebootCause_);
}

const char* DeviceLogManager::rebootCause() const {
    return rebootCause_;
}

bool DeviceLogManager::hasRebootCause() const {
    return rebootCause_[0] != '\0';
}

// ---------------------------------------------------------------------------
// Кольцевой лог на LittleFS
// ---------------------------------------------------------------------------

void DeviceLogManager::write(const char* fmt, ...) {
    char line[LINE_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    // Только Serial (не блокируемся на FS, если она не смонтирована)
    Serial.print("[device-log] ");
    Serial.println(line);

    if (!fsMounted_) {
        return;
    }

    // Форму emplate строки: "[uptimeMs] message\n"
    char entry[LINE_BUF_SIZE + 24];
    snprintf(entry, sizeof(entry), "[%lu] %s\n", (unsigned long)millis(), line);

    File file = LittleFS.open(LOG_FILE, FILE_APPEND);
    if (!file) {
        Serial.println("[device-log] cannot open log for append");
        return;
    }
    file.print(entry);
    file.close();

    // Кольцевая ротация: если размер превысил лимит — обрезаем, оставляя хвост.
    File info = LittleFS.open(LOG_FILE, "r");
    if (!info) {
        return;
    }
    size_t size = info.size();
    info.close();
    if (size <= LOG_MAX_SIZE) {
        return;
    }

    // Перечитываем последние LOG_MAX_SIZE байт и пересоздаём файл с ними.
    File src = LittleFS.open(LOG_FILE, "r");
    if (!src) {
        return;
    }
    src.seek(size - LOG_MAX_SIZE);

    char tmp[LOG_MAX_SIZE];
    size_t readLen = src.readBytes(tmp, sizeof(tmp) - 1);
    src.close();
    tmp[readLen] = '\0';

    File dst = LittleFS.open(LOG_FILE, FILE_WRITE);
    if (dst) {
        dst.print(tmp);
        dst.close();
    }
}

size_t DeviceLogManager::readTail(char* buf, size_t bufSize) {
    if (bufSize == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (!fsMounted_) {
        return 0;
    }

    File file = LittleFS.open(LOG_FILE, "r");
    if (!file) {
        return 0;
    }
    size_t size = file.size();
    size_t offset = size > bufSize - 1 ? size - (bufSize - 1) : 0;
    file.seek(offset);
    size_t readLen = file.readBytes(buf, bufSize - 1);
    file.close();
    buf[readLen] = '\0';
    return readLen;
}

void DeviceLogManager::clear() {
    if (fsMounted_) {
        LittleFS.remove(LOG_FILE);
    }
}
