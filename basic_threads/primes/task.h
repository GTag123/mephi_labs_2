#pragma once

#include <cstdint>
#include <mutex>
#include <set>
#include <atomic>

/*
 * Класс PrimeNumbersSet -- множество простых чисел в каком-то диапазоне
 */
class PrimeNumbersSet {
public:
    PrimeNumbersSet();

    // Проверка, что данное число присутствует в множестве простых чисел
    bool IsPrime(uint64_t number) const;

    // Получить следующее по величине простое число из множества
    uint64_t GetNextPrime(uint64_t number) const;

    /*
     * Найти простые числа в диапазоне [from, to) и добавить в множество
     * Во время работы этой функции нужно вести учет времени, затраченного на ожидание лока мюьтекса,
     * а также времени, проведенного в секции кода под локом
     */
    void AddPrimesInRange(uint64_t from, uint64_t to);

    // Посчитать количество простых чисел в диапазоне [from, to)
    size_t GetPrimesCountInRange(uint64_t from, uint64_t to) const;

    // Получить наибольшее простое число из множества
    uint64_t GetMaxPrimeNumber() const;

    // Получить суммарное время, проведенное в ожидании лока мьютекса во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeWaitingForMutex() const;

    // Получить суммарное время, проведенное в коде под локом во время работы функции AddPrimesInRange
    std::chrono::nanoseconds GetTotalTimeUnderMutex() const;
private:
    std::set<uint64_t> primes_;
    mutable std::mutex set_mutex_;
    std::atomic<uint64_t> nanoseconds_under_mutex_, nanoseconds_waiting_mutex_;
};
