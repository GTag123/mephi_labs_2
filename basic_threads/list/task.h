#pragma once

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <set>
#include <atomic>
#include <vector>
#include <iostream>

/*
 * Потокобезопасный связанный список.
 */
template<typename T>
class ThreadSafeList {
public:
    /*
     * Класс-итератор, позволяющий обращаться к элементам списка без необходимости использовать мьютекс.
     * При этом должен гарантироваться эксклюзивный доступ потока, в котором был создан итератор, к данным, на которые
     * он указывает.
     * Итератор, созданный в одном потоке, нельзя использовать в другом.
     */
    class Iterator {
    public:
        using pointer = T*;
        using value_type = T;
        using reference = T&;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        T& operator *() {
        }

        T operator *() const {
        }

        T* operator ->() {
        }

        const T* operator ->() const {
        }

        Iterator& operator ++() {
        }

        Iterator operator ++(int) {
        }

        Iterator& operator --() {
        }

        Iterator operator --(int) {
        }

        bool operator ==(const Iterator& rhs) const {
        }

        bool operator !=(const Iterator& rhs) const {
        }

    };

    /*
     * Получить итератор, указывающий на первый элемент списка
     */
    Iterator begin() {
    }

    /*
     * Получить итератор, указывающий на "элемент после последнего" элемента в списке
     */
    Iterator end() {
    }

    /*
     * Вставить новый элемент в список перед элементом, на который указывает итератор `position`
     */
    void insert(Iterator position, const T& value) {
    }

    /*
     * Стереть из списка элемент, на который указывает итератор `position`
     */
    void erase(Iterator position) {
    }
};
