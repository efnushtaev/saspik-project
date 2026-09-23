#include "DeviceLog.h"
#include <Arduino.h>
#include <Preferences.h>
#include <stdio.h>

DeviceLogManager DeviceLog;

// ---------------------------------------------------------------------------
// Конфигурация
// ---------------------------------------------------------------------------

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
    loadRebootCause();
    return true;
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
