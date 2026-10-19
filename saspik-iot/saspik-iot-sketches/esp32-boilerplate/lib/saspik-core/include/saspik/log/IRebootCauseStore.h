// SPDX-License-Identifier: MIT
/*
 * IRebootCauseStore.h — фасад над shared/device-log (причина последнего ребута).
 *
 * Извлекает за интерфейс синглтон DeviceLogManager из переиспользуемой
 * библиотеки: Logger зависит от интерфейса, а App подставляет реализацию,
 * внутри которой живёт глобальный объект (единственное документированное
 * исключение из «без глобалов», см. CaptivePortal.h).
 */

#pragma once

/**
 * @brief Чтение/запись причины последнего ребута (NVS).
 */
class IRebootCauseStore {
public:
    virtual ~IRebootCauseStore() = default;

    /** @brief Инициализация (чтение причины из NVS). */
    virtual bool begin() = 0;

    /** @brief true, если причина предыдущего ребута ещё не отправлена. */
    virtual bool hasRebootCause() const = 0;

    /** @brief Текст причины ("" если причины нет). */
    virtual const char* rebootCause() const = 0;

    /** @brief Сохранить причину перед авто-ребутом. */
    virtual void setRebootCause(const char* cause) = 0;
};