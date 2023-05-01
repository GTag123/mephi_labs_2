#pragma once

#include <string>

/*
 * Преобразовать 4 байта в формате big endian в int
 */
size_t BytesToInt(std::string_view bytes);

/*
 * Преобразовать int в 4 байта в формате big endian
 */
std::string IntToBytes(size_t value);

/*
 * Расчет SHA1 хеш-суммы. Здесь в результате подразумевается не человеко-читаемая строка, а массив из 20 байтов
 * в том виде, в котором его генерирует библиотека OpenSSL
 */
std::string CalculateSHA1(const std::string& msg);
