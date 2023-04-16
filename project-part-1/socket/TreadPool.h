#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <chrono>
class ThreadPool {
    size_t size;
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    mutable std::mutex mutex;
    std::condition_variable isempty;
    std::condition_variable cvempty;
    std::atomic<bool> active;
    std::atomic<bool> isForceTerminated;
public:
    ThreadPool(size_t threadCount): size(threadCount), active(true), isForceTerminated(false) {
        for (size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back([this]() {
//                int count = 0;
                while (true) {
                    std::unique_lock lock(mutex);
                    while (tasks.empty() && active.load()) {
                        cvempty.wait(lock);
                    }
                    if (!active && tasks.empty()) {
                        this->isempty.notify_one();
                        return;
                    }
                    if (isForceTerminated.load()) {
                        return;
                    }
                    std::function<void()> task = tasks.front();
                    tasks.pop();
                    lock.unlock();
//                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    task();
                }
            });
        }
    }
    void PushTask(const std::function<void()>& task) {
        std::unique_lock lock(mutex);
        if (!(active.load())) {
            throw std::runtime_error("ThreadPool is not active");
        }
        tasks.push(task);
        cvempty.notify_one();
        lock.unlock();
    }

    void Terminate(bool wait) {
        std::unique_lock lock(mutex);
        active = false;
        if (wait) {
            while (!tasks.empty()) {
                isempty.wait(lock);
            }
        }
        else {
            isForceTerminated = true;
        }
        cvempty.notify_all();
        lock.unlock();
        for (auto& thread : threads) {
            thread.join();
        }
    }

    bool IsActive() const {
        return active;
    }

    size_t QueueSize() const {
        std::unique_lock lock(mutex);
        return tasks.size();
    }
};