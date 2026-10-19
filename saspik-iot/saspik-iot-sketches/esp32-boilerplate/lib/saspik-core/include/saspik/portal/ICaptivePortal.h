// SPDX-License-Identifier: MIT
/*
 * ICaptivePortal.h — фасад над shared/wifi-config (captive portal).
 *
 * Заголовок зависит от Arduino: метод begin() работает с DeviceConfig из
 * WifiConfig.h (переиспользуемая библиотека «как есть»). Интерфейс введён,
 * чтобы шаблоны приложений зависели от абстракции, а единственный глобальный
 * объект WifiConfig оставался внутри реализации CaptivePortal.
 */

#pragma once

#ifdef ARDUINO

#include <stdint.h>

#include "WifiConfig.h"  // DeviceConfig, shared/wifi-config (переиспользуем как есть)

/**
 * @brief Каптивный портал настройки WiFi/MQTT.
 */
class ICaptivePortal {
public:
    virtual ~ICaptivePortal() = default;

    /**
     * @brief Читает конфиг из NVS; при отсутствии или зажатой кнопке
     *        поднимает AP с порталом и возвращает false.
     * @param out       структура для заполнения (WiFi/MQTT)
     * @param buttonPin GPIO кнопки (-1 если нет), INPUT_PULLUP, нажатие = LOW
     * @param defaults  значения по умолчанию для формы (или nullptr)
     * @return true — устройство подключено к WiFi, false — работает портал
     */
    virtual bool begin(DeviceConfig& out, int8_t buttonPin,
                       const DeviceConfig* defaults) = 0;

    /** @brief Обслуживает портал (DNS+HTTP). Вызывать каждый loop(). */
    virtual void handlePortal() = 0;

    /** @brief true, пока активен режим портала. */
    virtual bool isPortalMode() const = 0;
};

#endif  // ARDUINO