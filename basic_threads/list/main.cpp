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

constexpr size_t elementsCount = 100000;
enum class Event {
    Erase = 0,
    Iterate
};

std::list<Event> events;
std::mutex eventsMutex;

void ThreadFunction(ThreadSafeList<ListItem> &list) {
    const auto thread_id = std::this_thread::get_id();
    {
        std::lock_guard<std::mutex> guard(coutMutex);
        std::cout << "Thread " << thread_id << " started" << std::endl;
    }

    size_t partSize = (elementsCount / 10);

    uint64_t sum = 0;
    size_t counter = 0;

    for (ListItem &item: list) {
        sum += item.value;
        ++counter;
        if (counter == 591) {
            std::cout << std::endl;
        }
        if (counter % partSize == 0) {
            item.reachOrder.push_back(thread_id);
            if (item.reachOrder.size() == 1) {
                {
                    std::lock_guard<std::mutex> guard(coutMutex);
                    std::lock_guard<std::mutex> guardEvents(eventsMutex);
                    std::cout << "Thread " << thread_id << " first reached sum " << sum << std::endl;
                    events.push_back(Event::Iterate);
                }
                std::this_thread::sleep_for(20ms * counter / partSize);
            } else {
                std::lock_guard<std::mutex> guard(coutMutex);
                std::lock_guard<std::mutex> guardEvents(eventsMutex);
                std::cout << "Thread " << thread_id << " reached sum " << sum << std::endl;
                events.push_back(Event::Iterate);
            }
        }
    }
    {
        std::lock_guard<std::mutex> guard(coutMutex);
        std::cout << "Thread " << thread_id << " finished with sum " << sum << std::endl;
    }
}

void EraseThreadFunction(ThreadSafeList<ListItem> &list) {
    const auto thread_id = std::this_thread::get_id();
    size_t counter = 0;
    const size_t partSize = (elementsCount / 10);
    for (ThreadSafeList<ListItem>::Iterator it = --list.end(); it != list.begin();) {
        ++counter;
        if (counter % partSize == 0) {
            auto iterToErase = it;
            --it;
            list.erase(iterToErase);
            std::lock_guard<std::mutex> guard(coutMutex);
            std::lock_guard<std::mutex> guardEvents(eventsMutex);
            std::cout << "Thread " << thread_id << " erased something" << std::endl;
            events.push_back(Event::Erase);
        } else {
            --it;
        }
    }
    {
        std::lock_guard<std::mutex> guard(coutMutex);
        std::cout << "Erase thread " << thread_id << " finished erase" << std::endl;
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
            threads.emplace_back([&list, threadsCount, i]() {
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

        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds"
                  << std::endl;

        size_t realElementsCount = 0;
        for (const auto &item: list) {
            std::ignore = item;
            ++realElementsCount;
        }
        assert(realElementsCount == elementsCount);
    }

    {
        std::cout << "Iterate list" << std::endl;

        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;

        for (size_t i = 0; i < threadsCount; ++i) {
            threads.emplace_back(ThreadFunction, std::ref(list));
        }
        threads.emplace_back(EraseThreadFunction, std::ref(list));

        for (auto &thread: threads) {
            thread.join();
        }

        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
        const std::chrono::duration duration = finishTime - startTime;
        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);

        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds"
                  << std::endl;

        size_t realElementsCount = 0;
        for (const auto &item: list) {
            std::ignore = item;
            ++realElementsCount;
        }
        assert(realElementsCount == elementsCount - 9);

        bool foundEraseBetweenIterations = false;
        for (auto it = events.begin(); !foundEraseBetweenIterations && it != events.end(); ++it) {
            auto next = std::next(it);
            if (next == events.end()) {
                break;
            }

            auto nextNext = std::next(next);
            if (nextNext == events.end()) {
                break;
            }

            if (*it == Event::Iterate && *next == Event::Erase && *nextNext == Event::Iterate) {
                foundEraseBetweenIterations = true;
            }
        }

        assert(foundEraseBetweenIterations);
    }

    std::list<std::thread::id> prevReachOrder;
    size_t count = 0;
    for (ThreadSafeList<ListItem>::Iterator it = list.begin(); it != list.end(); ++it) {
        if (!it->reachOrder.empty()) {
            ++count;
            std::cout << "Checkpoint #" << count << " threads order:\n";
            for (auto thread: it->reachOrder) {
                std::cout << thread << '\t';
            }
            std::cout << std::endl;
            assert(it->reachOrder != prevReachOrder);
            prevReachOrder = it->reachOrder;
        }
    }

    return 0;
}


//#include "task.h"
//
//#include <cassert>
//#include <vector>
//#include <list>
//#include <thread>
//#include <iostream>
//#include <random>
//
//using namespace std::chrono_literals;
//
//std::mutex coutMutex;
//
//struct ListItem {
//    uint64_t value;
//    std::list<std::thread::id> reachOrder;
//};
//
//constexpr size_t elementsCount = 10000;
//enum class Event {
//    Erase = 0,
//    Iterate
//};
//
//std::list<Event> events;
//std::mutex eventsMutex;
//
//void ThreadFunction(ThreadSafeList<ListItem> &list) {
//    const auto thread_id = std::this_thread::get_id();
//    {
//        std::lock_guard<std::mutex> guard(coutMutex);
//        std::cout << "Thread " << thread_id << " started" << std::endl;
//    }
//
//    size_t partSize = (elementsCount / 10);
//
//    uint64_t sum = 0;
//    size_t counter = 0;
//
//    for (ListItem &item: list) {
//        sum += item.value;
//        ++counter;
//        if (counter % partSize == 0) {
//            item.reachOrder.push_back(thread_id);
//            if (item.reachOrder.size() == 1) {
//                {
//                    std::lock_guard<std::mutex> guard(coutMutex);
//                    std::lock_guard<std::mutex> guardEvents(eventsMutex);
//                    std::cout << "Thread " << thread_id << " first reached sum " << sum << std::endl;
//                    events.push_back(Event::Iterate);
//                }
//                std::this_thread::sleep_for(20ms * counter / partSize);
//            } else {
//                std::lock_guard<std::mutex> guard(coutMutex);
//                std::lock_guard<std::mutex> guardEvents(eventsMutex);
//                std::cout << "Thread " << thread_id << " reached sum " << sum << std::endl;
//                events.push_back(Event::Iterate);
//            }
//        }
//    }
//    {
//        std::lock_guard<std::mutex> guard(coutMutex);
//        std::cout << "Thread " << thread_id << " finished with sum " << sum << std::endl;
//    }
//}
//
//void EraseThreadFunction(ThreadSafeList<ListItem> &list) {
//    const auto thread_id = std::this_thread::get_id();
//    size_t counter = 0;
//    const size_t partSize = (elementsCount / 10);
//    auto beg = list.begin();
//    for (ThreadSafeList<ListItem>::Iterator it = --list.end(); it != beg;) {
//        ++counter;
////        if (counter % partSize == 0) {
////            std::cout << std::endl;
////        }
//        if (counter % partSize == 0) {
//            auto iterToErase = it;
//            --it;
//            list.erase(iterToErase);
//            std::lock_guard<std::mutex> guard(coutMutex);
//            std::lock_guard<std::mutex> guardEvents(eventsMutex);
//            std::cout << "Thread " << thread_id << " erased something" << std::endl;
//            events.push_back(Event::Erase);
//        } else {
//            --it;
////            {
////                std::lock_guard<std::mutex> lock(coutMutex);
////                auto beg = list.begin();
////                auto beg2 = list.begin();
////                auto beg3 = list.begin();
////                auto beg4 = list.begin();
////                std::cout << (it != beg) << std::endl;
////                std::cout << (it != beg2) << std::endl;
////                std::cout << (it != beg3) << std::endl;
////                std::cout << (it != beg4) << std::endl;
////                std::cout << (it != list.begin()) << std::endl;
////                {
//////                    std::lock_guard<std::mutex> lock(coutMutex);
//////                    std::cout << "begin" << std::endl;
////                    std::cout << (it != list.begin()) << std::endl;
////                }
////                std::cout << (it != list.begin()) << std::endl;
////            }
//        }
//    }
//}
//
////int main() {
////    assert(std::thread::hardware_concurrency() > 1);
//////    const size_t threadsCount = 1;
////    const size_t threadsCount = std::min(std::max(std::thread::hardware_concurrency() + 1, 4U), 8U);
////    std::cout << "Max concurrent threads: " << std::thread::hardware_concurrency() << std::endl;
////    std::cout << "Threads to be created: " << threadsCount << std::endl;
////
////    std::chrono::seconds multithreadDuration;
////
////    ThreadSafeList<ListItem> list;
////
////    {
////        std::cout << "Fill list with values" << std::endl;
////
////        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
////        std::vector<std::thread> threads;
////        for (size_t i = 0; i < threadsCount; ++i) {
////            threads.emplace_back([&list, threadsCount, i]() {
////                std::mt19937_64 random(i);
////                for (size_t j = 0; j < (elementsCount / threadsCount); ++j) {
////                    list.insert(list.end(), {.value=random() % 10});
////                }
////            });
////        }
////
////        for (auto &thread: threads) {
////            thread.join();
////        }
////
////        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
////        const std::chrono::duration duration = finishTime - startTime;
////        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);
////
////        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds"
////                  << std::endl;
////
////        size_t realElementsCount = 0;
////        for (const auto &item: list) {
////            std::ignore = item;
////            ++realElementsCount;
////        }
////        assert(realElementsCount == elementsCount);
////    }
////
////    {
////        std::cout << "Iterate list" << std::endl;
////
////        const std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
////        std::vector<std::thread> threads;
////
////        for (size_t i = 0; i < threadsCount; ++i) {
////            threads.emplace_back(ThreadFunction, std::ref(list));
////        }
////        threads.emplace_back(EraseThreadFunction, std::ref(list));
////
////        for (auto &thread: threads) {
////            thread.join();
////        }
////
////        const std::chrono::time_point finishTime = std::chrono::high_resolution_clock::now();
////        const std::chrono::duration duration = finishTime - startTime;
////        multithreadDuration = std::chrono::duration_cast<std::chrono::seconds>(duration);
////
////        std::cout << threads.size() << " threads have run in " << multithreadDuration.count() << " seconds"
////                  << std::endl;
////
////        size_t realElementsCount = 0;
////        for (const auto &item: list) {
////            std::ignore = item;
////            ++realElementsCount;
////        }
////        assert(realElementsCount == elementsCount - 9);
////
////        bool foundEraseBetweenIterations = false;
////        for (auto it = events.begin(); !foundEraseBetweenIterations && it != events.end(); ++it) {
////            auto next = std::next(it);
////            if (next == events.end()) {
////                break;
////            }
////
////            auto nextNext = std::next(next);
////            if (nextNext == events.end()) {
////                break;
////            }
////
////            if (*it == Event::Iterate && *next == Event::Erase && *nextNext == Event::Iterate) {
////                foundEraseBetweenIterations = true;
////            }
////        }
////
////        assert(foundEraseBetweenIterations);
////    }
////
////    std::list<std::thread::id> prevReachOrder;
////    size_t count = 0;
////    for (ThreadSafeList<ListItem>::Iterator it = list.begin(); it != list.end(); ++it) {
////        if (!it->reachOrder.empty()) {
////            ++count;
////            std::cout << "Checkpoint #" << count << " threads order:\n";
////            for (auto thread: it->reachOrder) {
////                std::cout << thread << '\t';
////            }
////            std::cout << std::endl;
////            assert(it->reachOrder != prevReachOrder);
////            prevReachOrder = it->reachOrder;
////        }
////    }
////
////    return 0;
//////}
//int main() {
//    ThreadSafeList<int> list;
//    std::mutex coutM;
//    const int threadCount = 8;
//    std::vector<std::thread> threads;
//    for (int i = 0; i < threadCount; ++i) {
//        threads.emplace_back([&list, i](){
//            for (int j = 0; j < 100; ++j) {
//                list.insert(list.end(), (100*i + j));
//            }
//        });
//    }
//    for (auto& thread: threads) {
//        thread.join();
//    }
//    threads.clear();
//
////    std::cout << "Start checking" << std::endl;
////    int cnt = 0;
////    for (auto it = --list.end(); it != list.begin(); --it) {
////        std::cout << cnt << std::endl;
////        if (it.getCurrentNode()->mutex_.try_lock()) {
////            it.getCurrentNode()->mutex_.unlock();
////        } else {
////            std::cout << "Locked at pos: " << cnt << std::endl;
////        }
////        cnt++;
////    }
////    std::cout << "End checking" << std::endl;
//
//    for (int i = 0; i < 1; ++i) {
//        threads.emplace_back([&list, i](){
//            int counter = 0;
////            auto start = --list.end();
////            auto stop = list.begin();
//            for (auto it = --list.end(); it != list.begin(); --it){
//                {
//                    std::lock_guard<std::mutex> lock(coutMutex);
//                    std::cout << "Until *" << std::endl;
//                }
//
//                if (counter % 2 == 0){
//                    {
//                        std::lock_guard<std::mutex> lock(coutMutex);
//                        std::cout << "Until erase" << std::endl;
//                    }
//                    list.erase(it);
//                    {
//                        std::lock_guard<std::mutex> lock(coutMutex);
//                        std::cout << "After erase" << std::endl;
//                    }
//                }
//                ++counter;
//            }
//        });
//    }
//    for (auto& thread: threads) {
//        thread.join();
//    }
//    threads.clear();
////    for (int i = 0; i < threadCount; ++i) {
////        threads.emplace_back([&list, i](){
////            for (int j = 0; j < 100; ++j) {
////                list.insert(list.begin(), (100*i + j));
////            }
////        });
////    }
//    return 0;
//}