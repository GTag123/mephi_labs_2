#include "task.h"

#include <cassert>
#include <vector>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <sstream>

using namespace std::chrono_literals;

constexpr int maxValue = 1000;

std::mutex globalMutex;
std::unordered_map<std::thread::id, size_t> processedItemsCount;

void ProducerThread(UnbufferedChannel<int>& channel) {
    for (int i = 0; i < maxValue; ++i) {
        channel.Put(i);
    }
}

void FirstProcessorThread(UnbufferedChannel<int>& sourceValuesChannel, UnbufferedChannel<std::string>& processedValuesChannel) {
    try {
        while (true) {
            const int value = sourceValuesChannel.Get(1000ms);
            std::stringstream ss;
            ss << "Value " << value << " processed in thread " << std::this_thread::get_id();
            processedValuesChannel.Put(ss.str());
            std::lock_guard<std::mutex> lock(globalMutex);
            processedItemsCount[std::this_thread::get_id()]++;
        }
    } catch (const TimeOut& ex) {
        std::lock_guard<std::mutex> lock(globalMutex);
        std::cout << "Thread " << std::this_thread::get_id() << " cannot read data from the source values channel" << std::endl;
    }
}

void SecondProcessorThread(UnbufferedChannel<std::string>& processedValuesChannel) {
    try {
        while (true) {
            const std::string value = processedValuesChannel.Get(1000ms);
            std::lock_guard<std::mutex> lock(globalMutex);
            std::cout << "Thread " << std::this_thread::get_id() << " has read this data from processed channel: " << value << std::endl;
        }
    } catch (const TimeOut& ex) {
        std::lock_guard<std::mutex> lock(globalMutex);
        std::cout << "Thread " << std::this_thread::get_id() << " cannot read data from the source values channel" << std::endl;
    }
}

int main() {
    assert(std::thread::hardware_concurrency() > 1);
    const size_t threadsCount = std::min(std::max(std::thread::hardware_concurrency() + 1, 4U), 8U);
    std::cout << "Max concurrent threads: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "Threads to be created: " << threadsCount << std::endl;

    UnbufferedChannel<int> sourceValuesChannel;
    UnbufferedChannel<std::string> processedValuesChannel;

    std::vector<std::thread> threads;

    const size_t processorThreadsCount = (threadsCount - 1) / 2;
    const size_t producerThreadsCount = threadsCount - processorThreadsCount - 1;

    assert(processorThreadsCount > 1);
    assert(producerThreadsCount > 1);

    for (size_t i = 0; i < producerThreadsCount; ++i) {
        threads.emplace_back(ProducerThread, std::ref(sourceValuesChannel));
    }
    for (size_t i = 0; i < processorThreadsCount; ++i) {
        threads.emplace_back(FirstProcessorThread, std::ref(sourceValuesChannel), std::ref(processedValuesChannel));
    }
    threads.emplace_back(SecondProcessorThread, std::ref(processedValuesChannel));

    assert(threads.size() == threadsCount);

    for (auto& thread : threads) {
        thread.join();
    }

    assert(processedItemsCount.size() == processorThreadsCount);
    size_t totalProcessedItems = 0;
    for (const auto& [threadId, processedItems] : processedItemsCount) {
        totalProcessedItems += processedItems;
    }

    assert(totalProcessedItems == maxValue * producerThreadsCount);

    for (const auto& [threadId, processedItems] : processedItemsCount) {
        assert(processedItems < 0.6 * totalProcessedItems);
    }

    return 0;
}
