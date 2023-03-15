#pragma once
#include "atomic"
#include "thread"
#include "condition_variable"

class mtx {
public:
    void lock() {
        bool expected = false;
        while(!_locked.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
            expected = false;
        }
    }

    void unlock() {
        _locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> _locked;
};


class SharedMutex
{
public:
    void lock_shared()
    {
        Mutex_.lock();
        while (ExclusiveCount_ != 0) {
            Mutex_.unlock();
            std::this_thread::yield();
            Mutex_.lock();
        }
        SharedCount_ += 1;
        Mutex_.unlock();
    }

    void unlock_shared()
    {
        Mutex_.lock();
        SharedCount_ -= 1;
        Mutex_.unlock();
    }

    void lock()
    {
        Mutex_.lock();
        while (SharedCount_ + ExclusiveCount_ != 0) {
            Mutex_.unlock();
            std::this_thread::yield();
            Mutex_.lock();
        }
        ExclusiveCount_ += 1;
        Mutex_.unlock();
    }

    void unlock()
    {
        Mutex_.lock();
        ExclusiveCount_ -= 1;
        Mutex_.unlock();
    }

private:
    mtx Mutex_;
    std::condition_variable Available_;
    int ExclusiveCount_ = 0;
    int SharedCount_ = 0;
};

//class SharedMutex {
//    std::atomic<int> sharedCounter = 0;
//    std::atomic<bool> isUniqueLocked = false;
//public:
//    void lock() {
//        int expectedShared = 0;
//        bool expectedUnique = false;
//        while(!sharedCounter.compare_exchange_weak(expectedShared, 0) &&
//            !isUniqueLocked.compare_exchange_weak(expectedUnique, true)){
//            expectedUnique = false;
//            expectedShared = false;
//        }
//    }
//    void unlock() {
//        isUniqueLocked.store(false);
////        isSharedLocked.store(false); // хз нужно ли
//    }
//
//    void lock_shared() {
//        bool expectedUnique = false;
//        while (!isUniqueLocked.compare_exchange_weak(expectedUnique, true)){
//            expectedUnique = false;
//        }
//    }
//
//    void unlock_shared() {
//    }
//};
