#include "task.h"

#include <cassert>
#include <iostream>
#include <chrono>

using namespace std::literals::chrono_literals;

/*
 * Складывает числа на полуинтервале [from, to)
 */
uint64_t SumNumbers(uint64_t from, uint64_t to) {
    uint64_t sum = 0;
    for (uint64_t number = from; number < to; ++number) {
        sum += number;
    }
    return sum;
}

void TestSimple() {
    ThreadPool pool(10);

    std::mutex mutex;
    uint64_t sum = 0;

    constexpr int step = 1000;
    constexpr uint64_t maxNumber = 500000000;
    for (uint64_t l = 0, r = l + step; l <= maxNumber; l = r, r = l + step) {
        if (r > maxNumber + 1) {
            r = maxNumber + 1;
        }
        pool.PushTask([&sum, &mutex, l, r]() {
            std::this_thread::sleep_for(100us);
            const uint64_t subsum = SumNumbers(l, r);
            std::lock_guard<std::mutex> lockGuard(mutex);
            sum += subsum;
        });
    }

    std::cout << "QueueSize before terminate is " << pool.QueueSize() << std::endl;
    assert(pool.QueueSize() > 100000);
    pool.Terminate(true);
    std::cout << "Terminated. Queue size is " << pool.QueueSize() << ". IsActive: " << pool.IsActive() << std::endl;

    const uint64_t expectedSum = SumNumbers(1, maxNumber + 1);
    assert(expectedSum == sum);

    try {
        pool.PushTask([](){
            std::cout << "I am a new task!" << std::endl;
        });
        assert(false);
    } catch (const std::exception& e) {
        std::cout << "Cannot push tasks after termination" << std::endl;
    }
}

void TestTerminationWithoutWait() {
    ThreadPool pool(10);

    std::mutex mutex;
    uint64_t sum = 0;

    constexpr int step = 1000;
    constexpr uint64_t maxNumber = 500000000;
    for (uint64_t l = 0, r = l + step; l <= maxNumber; l = r, r = l + step) {
        if (r > maxNumber + 1) {
            r = maxNumber + 1;
        }
        pool.PushTask([&sum, &mutex, l, r]() {
            std::this_thread::sleep_for(100us);
            const uint64_t subsum = SumNumbers(l, r);
            std::lock_guard<std::mutex> lockGuard(mutex);
            sum += subsum;
        });
    }

    std::cout << "QueueSize before terminate is " << pool.QueueSize() << std::endl;
    assert(pool.QueueSize() > 100000);
    pool.Terminate(false);
    std::cout << "Terminated. Queue size is " << pool.QueueSize() << ". IsActive: " << pool.IsActive() << std::endl;

    const uint64_t expectedSum = SumNumbers(1, maxNumber + 1);
    assert(expectedSum > sum);

    try {
        pool.PushTask([](){
            std::cout << "I am a new task!" << std::endl;
        });
        assert(false);
    } catch (const std::exception& e) {
        std::cout << "Cannot push tasks after termination" << std::endl;
    }
}

void TestConcurrentTaskPush() {
    ThreadPool pool(10);

    std::atomic<int> counter = 0;

    std::vector<std::thread> pusherThreads;
    for (int i = 0; i < 1000; ++i) {
        pusherThreads.emplace_back([&](){
            std::this_thread::sleep_for(10us);
            for (int j = 0; j < 1000; ++j) {
                pool.PushTask([&](){
                    std::this_thread::sleep_for(5us);
                    counter++;
                });
            }
        });
    }

    for (auto& thread : pusherThreads) {
        thread.join();
    }
    pool.Terminate(true);

    assert(counter == 1000000);
}

void TestConcurrentSelfTaskPush() {
    ThreadPool pool(10);

    std::atomic<int> counter = 0;
    std::atomic<int> pusherThreadDoneCounter = 0;

    for (int i = 0; i < 1000; ++i) {
        pool.PushTask([&](){
            std::this_thread::sleep_for(10us);
            for (int j = 0; j < 1000; ++j) {
                pool.PushTask([&](){
                    std::this_thread::sleep_for(10us);
                    counter++;
                });
            }
            pusherThreadDoneCounter++;
        });
    }

    while(pusherThreadDoneCounter != 1000) {
        std::this_thread::sleep_for(10ms);
    }
    pool.Terminate(true);

    assert(counter == 1000000);
}

int main() {
    TestSimple();
    TestTerminationWithoutWait();
    TestConcurrentSelfTaskPush();
    TestConcurrentTaskPush();

    return 0;
}
