#pragma once

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <vector>
#include <iostream>
#include "memory"

/*
 * Потокобезопасный связанный список.
 */

template<typename T>
struct TNode {
    T value;
    TNode<T> *next;
    TNode<T> *prev;
    mutable std::shared_mutex mutex_;
    std::atomic<bool> isRefLocked = false;

    TNode(T v, TNode<T> *n, TNode<T> *p) : value(std::move(v)), next(std::move(n)), prev(std::move(p)) {}

//    ~TNode() {
//        mutex_.lock();
//        mutex_.unlock();
//    }
};

template<typename T>
class ThreadSafeList {
private:
    std::shared_mutex headListMutex;
    std::shared_mutex tailListMutex;

    TNode<T> *head = nullptr;
    TNode<T> *tail = nullptr;
    friend class Iterator;

public:
    ThreadSafeList() {};

    class Iterator {
    private:
        TNode<T> *curr_;
        ThreadSafeList& list_;

        TNode<T> *getCurrentNode() const {
//            if (curr_ != nullptr) .lock() по идее итератор используется в одном потоке так что всё должно быть норм
            return curr_;
        }

        friend ThreadSafeList;
    public:
        using pointer = T *;
        using value_type = T;
        using reference = T &;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(TNode<T> *current, ThreadSafeList& list) : curr_(std::move(current)), list_(list) {
//            curr_->mutex_.lock();
        };

        ~Iterator(){
            if (curr_ != nullptr){
                if (curr_->isRefLocked) curr_->isRefLocked = false;
                curr_->mutex_.unlock();
            }
        }

        T& operator*() {

//            std::cout << "T& operator*" << std::endl;
//            curr_->isRefOperator = true;
            curr_->mutex_.lock();
            curr_->isRefLocked = true;
//            std::unique_lock<std::shared_mutex> ulock(curr_->mutex_);
            return (curr_->value);

        }

        T operator*() const {
//            std::cout << "T operator*" << std::endl;
            std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
            return curr_->value;
        }

//        T* operator->() {
////            std::cout << "T* operator->" << std::endl;
//            std::unique_lock<std::shared_mutex> ulock(curr_->mutex_);
//            return &(curr_->value);
//        }

        const T* operator->() const {
//            std::cout << "const T* operator->" << std::endl;

            std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
            return &(curr_->value);
        }

        Iterator &operator++() {
//            std::cout << "It operator++" << std::endl;
            if (curr_->isRefLocked){
                curr_->isRefLocked = false;
                auto prev = curr_;
                curr_ = curr_->next;
                prev->mutex_.unlock();
                return *this;
            } else {
                std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
                curr_ = curr_->next;
                return *this;
            }
        }
//        Iterator operator++(int) {
////            std::cout << "Operator++(int)" << std::endl;
//            std::unique_lock<std::shared_mutex> ulock(curr_->mutex_);
//            auto old = curr_;
//            curr_ = curr_->next;
//            return old;
//        }
//
        Iterator &operator--() {
            if (curr_ == nullptr){
                std::shared_lock<std::shared_mutex> lock(list_.tailListMutex);
                curr_ = list_.tail;
                return *this;
            }
            if (curr_->isRefLocked){
                curr_->isRefLocked = false;
                auto next = curr_;
                curr_ = curr_->prev;
                next->mutex_.unlock();
                return *this;
            } else {
                std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
                curr_ = curr_->prev;
                return *this;
            }
        }

//            std::unique_lock<std::shared_mutex> ulock(curr_->mutex_);
//            curr_ = curr_->prev;
//            return *this;
//        }
//
//        Iterator operator--(int) {
//            std::unique_lock<std::shared_mutex> ulock(curr_->mutex_);
//            auto old = curr_;
//            curr_ = curr_->prev;
//            return old;
//        }

//        bool operator==(const Iterator &rhs) const {
//            if (curr_ == nullptr) return rhs.curr_ == nullptr;
//            std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
//            return curr_ == rhs.curr_;
//        }

        bool operator!=(const Iterator &rhs) const {
            if (curr_ == nullptr) return rhs.curr_ != nullptr;
            std::shared_lock<std::shared_mutex> common_lock(curr_->mutex_);
            return curr_ != rhs.curr_;
        }

    };

    Iterator begin() {
        std::shared_lock<std::shared_mutex> headlock(headListMutex);
        if (head == nullptr) return Iterator(nullptr, *this);
        std::shared_lock<std::shared_mutex> nodelock(head->mutex_);
        return Iterator(head, *this);
    }

    Iterator end() {
        return Iterator(nullptr, *this); // либо tail хз
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T &value) {
        std::shared_lock<std::shared_mutex> shHeadLock(headListMutex);
        std::shared_lock<std::shared_mutex> shTailLock(tailListMutex);
        if (head == nullptr && tail == nullptr) {
            shHeadLock.unlock();
            shTailLock.unlock();
            auto newNode = new TNode<T>(T(value), nullptr, nullptr);
            std::cout << "Created value" << std::endl;
            std::unique_lock<std::shared_mutex> uHeadLock(headListMutex);
            head = newNode;
            std::cout << "Created value 2" << std::endl;
            {
                std::unique_lock<std::shared_mutex> uTailLock(tailListMutex);
                tail = newNode;
            }
            std::cout << "Created value 3" << std::endl;
            return;
        }
//        std::shared_lock<std::shared_mutex> shPositionLock(position.getCurrentNode()->mutex_);
        if (position.getCurrentNode() == nullptr) {
            shHeadLock.unlock();
            shTailLock.unlock();
//            shPositionLock.unlock();
            std::unique_lock<std::shared_mutex> uTailLock(tailListMutex);
            auto newNode = new TNode<T>(T(value), nullptr, tail);
//            std::cout << "Tail2, pos - nullptr" << std::endl;
            std::unique_lock<std::shared_mutex> uTailElemLock(tail->mutex_);
            tail->next = newNode;
            tail = newNode;
        } else {
            std::cout << "hui" << std::endl;
//            shPositionLock.unlock();
            std::unique_lock<std::shared_mutex> uPositionLock(position.getCurrentNode()->mutex_);

            if (position.getCurrentNode()->prev != nullptr) {
                position.getCurrentNode()->prev->mutex_.lock();
            }
            auto newNode = new TNode<T>(T(value), position.getCurrentNode(),
                                        position.getCurrentNode()->prev);
            std::unique_lock<std::shared_mutex> uNewNodeLock(newNode->mutex_);

            if (position != this->begin()) { // вроде безопасно
                position.getCurrentNode()->prev->next = newNode;
            } else {
                std::unique_lock<std::shared_mutex> uHeadLock(headListMutex);
                head = newNode;
            }
            if (position.getCurrentNode()->prev != nullptr) position.getCurrentNode()->prev->mutex_.unlock();
            position.getCurrentNode()->prev = newNode;
        }
    }

    void erase(Iterator position) {
        std::cout << "-----------------erase____________" << std::endl;
//        delete position.getCurrentNode()->value; я хз где правильнее
        std::unique_lock<std::shared_mutex> uPositionLock(position.getCurrentNode()->mutex_);
        if (position.getCurrentNode()->prev != nullptr) {
            std::unique_lock<std::shared_mutex> uPositionPrevLock(position.getCurrentNode()->prev->mutex_);
            position.getCurrentNode()->prev->next = position.getCurrentNode()->next;
        } else {
            std::unique_lock<std::shared_mutex> uHeadLock(headListMutex);
            head = position.getCurrentNode()->next;
        }
        if (position.getCurrentNode()->next != nullptr) {
            std::unique_lock<std::shared_mutex> uPositionNextLock(position.getCurrentNode()->next->mutex_);
            position.getCurrentNode()->next->prev = position.getCurrentNode()->prev;
        } else {
            std::unique_lock<std::shared_mutex> uTailLock(tailListMutex);
            tail = position.getCurrentNode()->prev;
        }
        uPositionLock.unlock();
        delete position.getCurrentNode(); // хз насколько безопасно
    }
};
