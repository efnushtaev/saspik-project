// SPDX-License-Identifier: MIT
/*
 * CaptivePortal.cpp — делегирование глобальному WifiConfig.
 *
 * #ifdef ARDUINO: shared/wifi-config тянет Arduino (WString/WebServer),
 * на хосте файл компилируется как пустая единица трансляции.
 *
 * Важные факты поведения переиспользуемой библиотеки (проверены по коду):
 *  - begin() блокирует до 20 с на STA-подключение либо поднимает AP+портал;
 *  - POST /save пишет конфиг в NVS и сам делает ESP.restart();
 *  - POST /reset чистит NVS и перезагружается.
 * Шаблоны приложений полагаются на эти гарантии (см. App::begin()).
 */

#include "portal/CaptivePortal.h"

#ifdef ARDUINO

#include "WifiConfig.h"  // объявляет синглтон WifiConfigManager WifiConfig

bool CaptivePortal::begin(DeviceConfig& out, int8_t buttonPin,
                          const DeviceConfig* defaults) {
    return ::WifiConfig.begin(out, buttonPin, defaults);
}

void CaptivePortal::handlePortal() { ::WifiConfig.handlePortal(); }

bool CaptivePortal::isPortalMode() const { return ::WifiConfig.isPortalMode(); }

#endif  // ARDUINO