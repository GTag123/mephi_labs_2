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
    mutable std::shared_mutex valuemutex_; // for value and isreflock
    mutable std::shared_mutex nodemutex_; // for prev and next and isDeleted
    std::atomic<bool> isRefLocked;
    std::atomic<bool> isTail;
    std::atomic<bool> isDeleted; // попробовать убрать атомики если смогу сделать лабу
    TNode(T v, TNode<T> *n, TNode<T> *p) : value(std::move(v)), next(std::move(n)), prev(std::move(p)), isRefLocked(false) {}
    TNode(TNode<T> *n, TNode<T> *p) : next(std::move(n)), prev(std::move(p)), isRefLocked(false), isTail(true) {}

//    ~TNode() {
//        mutex_.lock();
//        mutex_.unlock();
//    }
};

template<typename T>
class ThreadSafeList {
    mutable std::shared_mutex headListMutex;
//    std::shared_mutex tailListMutex;

    TNode<T> *head = nullptr;
    TNode<T> *tail = nullptr;
public:
    /*
     * Класс-итератор, позволяющий обращаться к элементам списка без необходимости использовать мьютекс.
     * При этом должен гарантироваться эксклюзивный доступ потока, в котором был создан итератор, к данным, на которые
     * он указывает.
     * Итератор, созданный в одном потоке, нельзя использовать в другом.
     */
    ThreadSafeList(){
        TNode<T>* tail_ = new TNode<T>(nullptr, nullptr);
        std::unique_lock<std::shared_mutex> headlock(headListMutex);
//        std::unique_lock<std::shared_mutex> taillock(tailListMutex);
        head = tail_;
        tail = tail_;
    }
    class Iterator {
        friend class ThreadSafeList;
        TNode<T>* curr_;
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator(TNode<T>* node): curr_(std::move(node)){};

        ~Iterator(){
            if (curr_ != nullptr) {
                if (curr_->isRefLocked) {
                    curr_->isRefLocked = false;
                    curr_->valuemutex_.unlock();
                }
                if (curr_->isDeleted) {
                    delete curr_;
                }
            }

        }

        TNode<T> getCurrentNode() const{
            return curr_;
        }
        T& operator *() {
            curr_->valuemutex_.lock();
            curr_->isRefLocked = true;
            return curr_->value;
        }

        T operator *() const {
            std::shared_lock<std::shared_mutex> lock(curr_->valuemutex_);
            return curr_->value;
        }

        T* operator ->() {
            std::shared_lock<std::shared_mutex> lock(curr_->valuemutex_);
            return &(curr_->value);
        }

        const T* operator ->() const {
            std::shared_lock<std::shared_mutex> lock(curr_->valuemutex_);
            return &(curr_->value);
        }

        Iterator& operator ++() {
            // если другой поток ++?
            std::unique_lock<std::shared_mutex> lock(curr_->nodemutex_);
            if (curr_->isRefLocked) {
                curr_->isRefLocked = false;
                curr_->valuemutex_.unlock();
            }
            auto old = curr_;
            curr_ = curr_->next;
            lock.unlock();

            if (old->isDeleted) {
                delete old;
            }
            return *this;
        }

        Iterator& operator --() {
            // если другой поток --?
            std::unique_lock<std::shared_mutex> lock(curr_->nodemutex_);
            if (curr_->isRefLocked) {
                curr_->isRefLocked = false;
                curr_->valuemutex_.unlock();
            }
            auto old = curr_;
            curr_ = curr_->prev;
            lock.unlock();
            if (old->isDeleted) {
                delete old;
            }
            return *this;
        }

        bool operator !=(const Iterator& rhs) const {
            return this->curr_ != rhs.curr_;
        }
        bool operator== (const Iterator& rhs) const {
            return !operator!=(rhs);
        }
    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() const {
        std::shared_lock<std::shared_mutex> lock(headListMutex);
        return Iterator(head);
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() const {
//        std::shared_lock<std::shared_mutex> lock(tail->nodemutex_);
        return Iterator(tail);
    }
    Iterator cend() const {
        return Iterator(tail);


        
    }
    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
        std::shared_lock<std::shared_mutex> shHeadLock(headListMutex);
//        std::shared_lock<std::shared_mutex> shTailLock(tailListMutex); // UNLOCK!
        if (head->isTail && tail->isTail) {
            auto newNode = new TNode<T>(T(value), tail, nullptr);
            shHeadLock.unlock();
            std::unique_lock<std::shared_mutex> taillock(tail->nodemutex_);
            tail->prev = newNode;
            std::unique_lock<std::shared_mutex> headlock(headListMutex);
            head = newNode;
        } else if (position == end()){
            shHeadLock.unlock();
            std::unique_lock<std::shared_mutex> taillock(tail->nodemutex_);
            std::unique_lock<std::shared_mutex> tailprevlock(tail->prev->nodemutex_);
            auto newNode = new TNode<T>(T(value), tail, tail->prev);
            tail->prev->next = newNode;
            tail->prev = newNode;
        } else {
            std::cout << "!!!!!!!!!!!!!!!!!!!!!!!SAASFSS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
        }
    }

    /*
     * Стереть из списка элемент, на который указывает итератор `position`
     */
    void erase(Iterator& position) {
        if (position == end()) { // заменить на end()
            throw std::invalid_argument("position is end()");
        }

        std::unique_lock<std::shared_mutex> lockPosNode(position.curr_->nodemutex_);
        if (position.curr_->prev != nullptr) {
            position.curr_->prev->nodemutex_.lock();
        } else {
            if (position != begin()) throw std::invalid_argument("position prev is nullptr, but position not begin()");
            headListMutex.lock();
        }
        if (position.curr_->next == nullptr) throw std::invalid_argument("position next is nullptr");
        position.curr_->next->nodemutex_.lock(); // костыль и будет ошибка
        if (position.curr_->prev != nullptr) {
            position.curr_->prev->next = position.curr_->next;
            position.curr_->prev->nodemutex_.unlock();
        } else {
            head = position.curr_->next;
            headListMutex.unlock();
        }
        position.curr_->next->nodemutex_.unlock();
        position.curr_->next->prev = position.curr_->prev;
        position.curr_->isDeleted = true;
        lockPosNode.unlock();
//        delete position.curr_; // TODO; мб delete после ++ -- и в деструкторе
    }
};
