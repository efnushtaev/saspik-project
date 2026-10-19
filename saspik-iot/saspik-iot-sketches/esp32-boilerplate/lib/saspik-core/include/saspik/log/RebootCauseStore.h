// SPDX-License-Identifier: MIT
/*
 * RebootCauseStore.h — реализация IRebootCauseStore поверх shared/device-log.
 *
 * Единственный документированный синглтон (DeviceLogManager DeviceLog) живёт
 * здесь, за фасадом: весь остальной код работает с IRebootCauseStore и не
 * знает про глобальный объект. Библиотеку менять нельзя («переиспользуем как есть»).
 */

#pragma once

#include "log/IRebootCauseStore.h"

/**
 * @brief Фасад над DeviceLogManager из shared/device-log.
 */
class RebootCauseStore : public IRebootCauseStore {
public:
    bool begin() override;
    bool hasRebootCause() const override;
    const char* rebootCause() const override;
    void setRebootCause(const char* cause) override;
};