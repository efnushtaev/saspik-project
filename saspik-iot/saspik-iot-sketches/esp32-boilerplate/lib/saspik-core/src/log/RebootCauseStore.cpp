// SPDX-License-Identifier: MIT
/*
 * RebootCauseStore.cpp — делегирование глобальному DeviceLog (shared/device-log).
 *
 * #ifdef ARDUINO: заголовок shared-библиотеки тянет Arduino, поэтому на хосте
 * файл компилируется как пустая единица трансляции.
 */

#include "log/RebootCauseStore.h"

#ifdef ARDUINO

#include "DeviceLog.h"  // shared/device-log (переиспользуем как есть)

bool RebootCauseStore::begin() { return DeviceLog.begin(); }

bool RebootCauseStore::hasRebootCause() const { return DeviceLog.hasRebootCause(); }

const char* RebootCauseStore::rebootCause() const { return DeviceLog.rebootCause(); }

void RebootCauseStore::setRebootCause(const char* cause) {
    DeviceLog.setRebootCause(cause);
}

#endif  // ARDUINO