#include "task.h"
#include "cmath"
#include "algorithm"
#include "iostream"
PrimeNumbersSet::PrimeNumbersSet() {
    this->nanoseconds_under_mutex_ = 0;
    this->nanoseconds_waiting_mutex_ = 0;
    this->primes_ = {};
//    this->set_mutex_;
};

bool PrimeNumbersSet::IsPrime(uint64_t number) const {
    if (number == 1) return false;
    if (number == 2) return true;
    if (number % 2 == 0) return false;
    for (uint64_t i = 3; i*i <= number; i+=2) {
        if (number % i == 0) return false;
    }
    return true;
}

std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeUnderMutex() const {
    return std::chrono::nanoseconds(nanoseconds_under_mutex_);
}

std::chrono::nanoseconds PrimeNumbersSet::GetTotalTimeWaitingForMutex() const {
    return std::chrono::nanoseconds(nanoseconds_waiting_mutex_);
}

uint64_t PrimeNumbersSet::GetNextPrime(uint64_t number) const {
    std::lock_guard<std::mutex> lg(set_mutex_);
    auto it = std::upper_bound(primes_.begin(), primes_.end(), number);
    if (it == primes_.end()) throw std::invalid_argument("Don't know next prime after limit\n");
    return *(it);
}


void PrimeNumbersSet::AddPrimesInRange(uint64_t from, uint64_t to) {
    std::set<uint64_t> localprimes;

    for (auto i = from; i < to; ++i) {
        if (IsPrime(i)){
            localprimes.insert(i);
        }
    }

    auto start = std::chrono::steady_clock::now();
    set_mutex_.lock();
    auto end = std::chrono::steady_clock::now();
    nanoseconds_waiting_mutex_ += (end - start).count();

    start = std::chrono::steady_clock::now();
    for (auto it: localprimes){
        primes_.insert(it);
    }
    end = std::chrono::steady_clock::now();
    std::cout << "until nanoseconds_under_mutex_: " << nanoseconds_under_mutex_ << std::endl;
    nanoseconds_under_mutex_ += (end - start).count();
    std::cout << "after nanoseconds_under_mutex_: " << nanoseconds_under_mutex_ << std::endl;
    set_mutex_.unlock();

}
//void PrimeNumbersSet::AddPrimesInRange(uint64_t from, uint64_t to) {
//    for (auto i = from; i < to; ++i) {
//        if (IsPrime(i)){
//            auto start = std::chrono::steady_clock::now();
//            set_mutex_.lock();
//            auto end = std::chrono::steady_clock::now();
//            nanoseconds_waiting_mutex_ += (end - start).count();
//
//            start = std::chrono::steady_clock::now();
//            primes_.insert(i);
//            set_mutex_.unlock();
//            end = std::chrono::steady_clock::now();
//            nanoseconds_under_mutex_ += (end - start).count();
//        }
//    }
//}

//void PrimeNumbersSet::AddPrimesInRange(uint64_t from, uint64_t to) {
//    auto start = std::chrono::steady_clock::now();
//    set_mutex_.lock();
//    auto end = std::chrono::steady_clock::now();
//    nanoseconds_waiting_mutex_ += (end - start).count();
//
//    start = std::chrono::steady_clock::now();
//    for (auto i = from; i < to; ++i) {
//        if (IsPrime(i)){
//            primes_.insert(i);
//        }
//    }
//    set_mutex_.unlock();
//    end = std::chrono::steady_clock::now();
//    nanoseconds_under_mutex_ += (end - start).count();
//}

size_t PrimeNumbersSet::GetPrimesCountInRange(uint64_t from, uint64_t to) const {
    std::lock_guard<std::mutex> lg(set_mutex_);
    auto it1 = std::lower_bound(primes_.begin(), primes_.end(), from);
    auto it2 = std::upper_bound(primes_.begin(), primes_.end(), to);
    return std::distance(it1, it2);
}

uint64_t PrimeNumbersSet::GetMaxPrimeNumber() const {
    std::lock_guard<std::mutex> lg(set_mutex_);
    if (primes_.size() == 0) {
        return 0;
    }
    return *(primes_.rbegin());
}

