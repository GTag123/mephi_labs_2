#define _LIBCPP_MUTEX
#define _LIBCPP_SHARED_MUTEX
#define _LIBCPP___MUTEX_BASE
#include "task.h"
#undef _LIBCPP___MUTEX_BASE
#undef _LIBCPP_MUTEX
#undef _LIBCPP_SHARED_MUTEX

#include <typeinfo>

template<typename T, typename = void>
constexpr bool is_defined = false;

template<typename T>
constexpr bool is_defined<T, decltype(typeid(T))> = true;

namespace std {
    class mutex;
    class shared_mutex;
}

constexpr bool mutex_is_defined = is_defined<std::mutex>;
constexpr bool shared_mutex_is_defined = is_defined<std::shared_mutex>;

#include <cassert>
#include <vector>
#include <thread>
#include <iostream>
#include <sstream>
#include <queue>
#include <random>
#include <shared_mutex>
#include <algorithm>

using namespace std::chrono_literals;

SharedMutex coutMutex;

void TestSharedMutex() {
    SharedMutex numbersMutex, randomMutex;
    std::vector<int> numbers;
    std::atomic<bool> action = true;
    std::atomic<int> threadSharingLockCount = 0;
    int maxThreadSharingLockCount = 0;
    std::random_device dev;
    std::mt19937 random(dev());

    numbers.resize(1000, 0);

    std::vector<std::thread> threads;

    const auto insertNumbersInRandomPositions = [&](){
        while (action) {
            std::this_thread::sleep_for(10ms);
            std::lock_guard<SharedMutex> lock(numbersMutex);
            std::lock_guard<SharedMutex> randomLock(randomMutex);
            numbers[random() % numbers.size()]++;
        }
    };

    const auto calculateMedian = [&]() {
        while(action) {
            std::this_thread::sleep_for(1s);
            std::vector<int> copy;
            {
                std::shared_lock<SharedMutex> lock(numbersMutex);
                threadSharingLockCount++;
                copy = numbers;
                threadSharingLockCount--;
            }
            std::nth_element(copy.begin(), copy.begin() + copy.size() / 2, copy.end());
            std::lock_guard<SharedMutex> coutLock(coutMutex);
            std::cout << "Now numbers median is " << copy[copy.size() / 2] << std::endl;
        }
    };

    const auto calculateMean = [&]() {
        while(action) {
            std::this_thread::sleep_for(1s);
            {
                std::shared_lock<SharedMutex> lock(numbersMutex);
                threadSharingLockCount++;
                const auto sum = std::accumulate(numbers.begin(), numbers.end(), 0);
                std::lock_guard<SharedMutex> coutLock(coutMutex);
                std::cout << "Now numbers mean is " << sum / numbers.size() << std::endl;
                threadSharingLockCount--;
            }
        }
    };

    const auto calculateSum = [&]() {
        while(action) {
            std::this_thread::sleep_for(1s);
            {
                std::shared_lock<SharedMutex> lock(numbersMutex);
                threadSharingLockCount++;
                const auto sum = std::accumulate(numbers.begin(), numbers.end(), 0);
                std::lock_guard<SharedMutex> coutLock(coutMutex);
                std::cout << "Now numbers sum is " << sum << std::endl;
                threadSharingLockCount--;
            }
        }
    };

    threads.emplace_back([&threadSharingLockCount, &maxThreadSharingLockCount, &action](){
        while (action) {
            maxThreadSharingLockCount = std::max(threadSharingLockCount.load(), maxThreadSharingLockCount);
        }
    });

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(insertNumbersInRandomPositions);
    }

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(calculateMedian);
        threads.emplace_back(calculateMean);
        threads.emplace_back(calculateSum);
    }

    std::this_thread::sleep_for(10s);
    action = false;

    for (auto& thread : threads) {
        thread.join();
    }

    assert(maxThreadSharingLockCount > 1);
}

int main() {
    assert(std::thread::hardware_concurrency() > 1);
    assert(!mutex_is_defined);
    assert(!shared_mutex_is_defined);

    TestSharedMutex();

    return 0;
}
