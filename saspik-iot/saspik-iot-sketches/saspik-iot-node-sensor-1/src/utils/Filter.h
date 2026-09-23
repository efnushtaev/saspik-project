#ifndef FILTER_H
#define FILTER_H

#include <Arduino.h>
#include "config.h"

/**
 * @brief Утилиты фильтрации показаний датчиков (SRP)
 * 
 * Содержит функции для чтения отфильтрованных аналоговых и цифровых
 * значений с защитой от шумов и дребезга контактов.
 */
namespace Filter {

/**
 * @brief Чтение отфильтрованных аналоговых значений с использованием усеченного среднего
 * @param pin Аналоговый пин для чтения
 * @param result Ссылка для сохранения результата
 * @return true если успешно, false в противном случае
 */
bool readAnalog(int pin, float& result);

/**
 * @brief Чтение отфильтрованных цифровых значений с защитой от дребезга
 * @param pin Цифровой пин для чтения
 * @param result Ссылка для сохранения результата (0 или 1)
 * @return true если успешно, false в противном случае
 */
bool readDigital(int pin, float& result);

} // namespace Filter

#endif // FILTER_H