#pragma once

#include <stdlib.h>
#include <vector>

/// Provides an indexed free list with constant-time removals from anywhere
/// in the list without invalidating indices. T must be trivially constructible 
/// and destructible.
template <class T>
class FreeList
{
public:
    static const int FIXED_CAPACITY = 4096;

    /// Creates a new free list.
    FreeList();
    ~FreeList();

    /// Inserts an element to the free list and returns an index to it.
    int insert(const T& element);

    // Removes the nth element from the free list.
    void erase(int n);

    // Removes all elements from the free list.
    void clear();

    // Returns the range of valid indices.
    int range() const;

    // Returns the nth element.
    T& operator[](int n);
    T* at(int n);

    // Returns the nth element.
    const T& operator[](int n) const;

    int capacity_ = FIXED_CAPACITY;
private:
    struct FreeElement
    {
        T element;
        int next;

        FreeElement() {};
        FreeElement(T element) : element(element) {}
        FreeElement(int next) : next(next) {}
    };
    FreeElement *data;
    FreeElement *heap_data = nullptr;
    FreeElement static_data[FIXED_CAPACITY];

    int size_ = 0;
    int first_free;
};

template <class T>
FreeList<T>::FreeList(): first_free(-1)
{
    data = &(static_data[0]);
    size_ = 0;
}

template<class T>
FreeList<T>::~FreeList()
{
    if (heap_data != nullptr) {
        free(heap_data);
        heap_data = nullptr;
        data = nullptr;
    }
}

template <class T>
int FreeList<T>::insert(const T& element)
{
    if (first_free != -1)
    {
        const int index = first_free;
        first_free = data[first_free].next;
        data[index].element = element;
        size_ += 1;
        return index;
    }
    else
    {
        // FreeElement fe(element);
        // data.push_back(fe);
        // return static_cast<int>(data.size() - 1);

        FreeElement fe(element);
        if (size_ >= capacity_) {
            // dynamic allocation of a new array
            const int new_capacity = capacity_ * 2;
            if (size_ == FIXED_CAPACITY) {
                heap_data = (FreeElement*) malloc(new_capacity * sizeof(FreeElement));
                memcpy(heap_data, data, size_ * sizeof(FreeElement));
                data = heap_data;
            } else {
                heap_data = (FreeElement*) realloc(heap_data, new_capacity * sizeof(FreeElement));
                data = heap_data;
            }
            capacity_ = new_capacity;
        }

        data[size_++] = fe;
        return size_-1;
        // return static_cast<int>(data.size() - 1);
    }
}

template <class T>
void FreeList<T>::erase(int n)
{
    data[n].next = first_free;
    size_ -= 1;
    first_free = n;
}

template <class T>
void FreeList<T>::clear()
{
    // data.clear()
    size_ = 0;
    first_free = -1;
}

template <class T>
int FreeList<T>::range() const
{
    return size_;
}

template <class T>
T& FreeList<T>::operator[](int n)
{
    return data[n].element;
}

template <class T>
const T& FreeList<T>::operator[](int n) const
{
    return data[n].element;
}