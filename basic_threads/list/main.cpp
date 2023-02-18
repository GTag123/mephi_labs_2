#include "task.h"

#include <cassert>
#include <vector>
#include <list>
#include <thread>
#include <iostream>
#include <random>

using namespace std::chrono_literals;

std::mutex coutMutex;

struct ListItem {
    uint64_t value;
    std::list<std::thread::id> reachOrder;
};

constexpr size_t elementsCount = 1000000;

void ThreadFunction(ThreadSafeList<ListItem>& list) {
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<std::mutex> guard(coutMutex);
        std::cout << "Thread " << thread_id << " started" << std::endl;
    }

    size_t partSize = (elementsCount / 10);

    uint64_t sum = 0;
    size_t counter = 0;

    for (ListItem& item : list) {
        sum += item.value;
        ++counter;
        if (counter % partSize == 0) {
            item.reachOrder.push_back(thread_id);
            if (item.reachOrder.size() == 1) {
                {
                    std::lock_guard<std::mutex> guard(coutMutex);
                    std::cout << "Thread " << thread_id << " first reached sum " << sum << std::endl;
                }
                std::this_thread::sleep_for(10ms * counter / partSize);
            } else {
                std::lock_guard<std::mutex> guard(coutMutex);
                std::cout << "Thread " << thread_id << " reached sum " << sum << std::endl;
            }
        }
    }
    {
        std::lock_guard<std::mutex> guard(coutMutex);
        std::cout << "Thread " << thread_id << " finished with sum " << sum << std::endl;
    }
}

int main() {
    assert(std::thread::hardware_concurrency() > 1);
    const size_t threadsCount = std::min(std::max(std::thread::hardware_concurrency() + 1, 4U), 8U);
    std::cout << "Max concurrent threads: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "Threads to be created: " << threadsCount << std::endl;

    std::chrono::seconds multithreadDuration;

    ThreadSafeList<ListItem> list;

    {
        std::cout << "Fill list with values" << std::endl;

        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        for (size_t i = 0; i < threadsCount; ++i) {
            threads.emplace_back([&list, threadsCount, i](){
                std::mt19937_64 random(i);
                for (size_t j = 0; j < (elementsCount / threadsCount); ++j) {
                    list.insert(list.end(), {.value=random() % 10});
                }
            });
        }

        for (auto &thread: threads) {
            thread.join();
        }

        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration duration = finishTime - startTime;
        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);

        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds" << std::endl;
    }

    {
        std::cout << "Iterate list" << std::endl;

        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;

        for (size_t i = 0; i < threadsCount; ++i) {
            threads.emplace_back(ThreadFunction, std::ref(list));
        }

        for (auto &thread: threads) {
            thread.join();
        }

        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration duration = finishTime - startTime;
        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);

        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds" << std::endl;
    }

    std::list<std::thread::id> prevReachOrder;
    size_t count = 0;
    for (ThreadSafeList<ListItem>::Iterator it = list.begin(); it != list.end(); ++it) {
        if (!it->reachOrder.empty()) {
            ++count;
            std::cout << "Checkpoint #" << count << " threads order:\n";
            for (auto thread : it->reachOrder) {
                std::cout << thread << '\t';
            }
            std::cout << std::endl;
            assert(it->reachOrder != prevReachOrder);
            prevReachOrder = it->reachOrder;
        }
    }

    return 0;
}
