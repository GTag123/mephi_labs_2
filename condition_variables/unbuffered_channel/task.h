#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>

class TimeOut : public std::exception {
    const char* what() const noexcept override {
            return "Timeout";
    }
};

template<typename T>
class UnbufferedChannel {
private:
    std::optional<T> data_;
    std::mutex mutex_;
    std::condition_variable full_;
    std::condition_variable empty_;
public:
    void Put(const T& data) {
        std::unique_lock lock(mutex_);
        while (data_ != std::nullopt){
            full_.wait(lock);
        }
        data_ = data;
        empty_.notify_one();
    }

    T Get(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) {
        auto timepoint = std::chrono::steady_clock::now() + timeout;
        std::unique_lock lock(mutex_);
        while (data_ == std::nullopt){
            if (timeout.count() != 0){
                if(empty_.wait_until(lock, timepoint) == std::cv_status::timeout) throw TimeOut();
            } else empty_.wait(lock);
        }
        const T value = data_.value(); // попробовать убрать конст
        data_ = std::nullopt;
        full_.notify_one();
        return value;
    }
};
