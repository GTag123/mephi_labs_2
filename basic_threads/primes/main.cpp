#include "task.h"

#include <cassert>
#include <vector>
#include <thread>
#include <iostream>

constexpr uint64_t limit = 50000000;
constexpr size_t expectedPrimesCount = 3001134;
constexpr size_t expectedMaxPrimeNumber = 49999991;

void ThreadFunction(PrimeNumbersSet& primes, uint64_t from, uint64_t to) {
    primes.AddPrimesInRange(from, to);
}

void CheckProperties(const PrimeNumbersSet& primes) {
    std::cout << "Has " << primes.GetPrimesCountInRange(0, limit) << " prime numbers\n";
    assert(primes.GetPrimesCountInRange(0, limit) == expectedPrimesCount);
    assert(!primes.IsPrime(0));
    assert(!primes.IsPrime(1));
    assert(primes.IsPrime(2));
    assert(primes.GetMaxPrimeNumber() == expectedMaxPrimeNumber);
    assert(primes.GetNextPrime(67) == 71);
    try {
        primes.GetNextPrime(primes.GetMaxPrimeNumber());
        assert(false);  // should never reach this line
    } catch (const std::invalid_argument& ex) {
        std::cout << "Don't know next prime after limit\n";
    }
}

int main() {
    using namespace std::chrono_literals;

    assert(std::thread::hardware_concurrency() > 1);
    const size_t threadsCount = std::min(std::thread::hardware_concurrency(), 8U);
    std::cout << "Concurrent threads: " << std::thread::hardware_concurrency() << std::endl;

    const uint64_t batchSize = limit / threadsCount;

    std::chrono::seconds multithreadDuration;

    {
        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
        PrimeNumbersSet primes;
        std::vector<std::thread> threads;

        for (uint64_t from = 0, to = batchSize; from < limit; from = to, to = std::min(to + batchSize, limit)) {
            threads.emplace_back(ThreadFunction, std::ref(primes), from, to);
        }

        size_t previousCount, newCount = 0;
        do {
            previousCount = newCount;
            newCount = primes.GetPrimesCountInRange(0, limit);
            std::cout << "Has " << newCount << " prime numbers\n";
            std::cout << "Max prime found: " << primes.GetMaxPrimeNumber() << std::endl;
            std::this_thread::sleep_for(2s);
        } while (newCount > previousCount);

        for (auto &thread: threads) {
            thread.join();
        }

        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration duration = finishTime - startTime;
        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);

        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds" << std::endl;
        std::cout << "Total waited for mutex: " << std::chrono::duration_cast<std::chrono::seconds>(primes.GetTotalTimeWaitingForMutex()).count() << std::endl;
        std::cout << "Total time under mutex: " << std::chrono::duration_cast<std::chrono::seconds>(primes.GetTotalTimeUnderMutex()).count() << std::endl;
        CheckProperties(primes);
        assert(primes.GetTotalTimeWaitingForMutex() > 5s);
        assert(primes.GetTotalTimeWaitingForMutex() > threadsCount * primes.GetTotalTimeUnderMutex() / 2);
    }

    {
        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
        PrimeNumbersSet primes;
        primes.AddPrimesInRange(0, limit);

        std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration duration = finishTime - startTime;

        std::cout << "1 thread has run in " << std::chrono::duration_cast<std::chrono::seconds>(duration).count() << " seconds" << std::endl;
        std::cout << "Total waited for mutex: " << std::chrono::duration_cast<std::chrono::seconds>(primes.GetTotalTimeWaitingForMutex()).count() << std::endl;
        std::cout << "Total time under mutex: " << std::chrono::duration_cast<std::chrono::seconds>(primes.GetTotalTimeUnderMutex()).count() << std::endl;
        CheckProperties(primes);
        assert(duration > multithreadDuration * 2);
        assert(primes.GetTotalTimeWaitingForMutex() < 5s);
    }

    return 0;
}
