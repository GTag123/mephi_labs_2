#include "task.h"

#include <cassert>
#include <vector>
#include <thread>
#include <iostream>

constexpr uint64_t limit = 10000000;
constexpr size_t expectedPrimesCount = 664579;
constexpr size_t expectedMaxPrimeNumber = 9999991;

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
    const size_t threadsCount = std::min(std::max(std::thread::hardware_concurrency() + 1, 4U), 8U);
    std::cout << "Max concurrent threads: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "Threads to be created: " << threadsCount << std::endl;

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
        assert(primes.GetTotalTimeWaitingForMutex() >= 4s);
        assert(primes.GetTotalTimeWaitingForMutex() > primes.GetTotalTimeUnderMutex());
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
        assert(duration >= multithreadDuration * 1.3);
        assert(primes.GetTotalTimeWaitingForMutex() < 2s);
    }

    return 0;
}
