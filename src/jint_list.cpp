#include "jint_list.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

JIntList::JIntList(int num_fields)
{
    this->data = &this->fixed[0];
    this->num = 0;
    this->cap = il_fixed_cap;
    this->num_fields = num_fields;
    this->free_element = -1;
}

JIntList::~JIntList()
{
    // Free the buffer only if it was heap allocated.
    if (this->data != this->fixed)
        free(this->data);
}

void JIntList::il_clear()
{
    this->num = 0;
    this->free_element = -1;
}

int JIntList::il_size()
{
    return this->num;
}

int JIntList::il_get(int n, int field)
{
    assert(n >= 0 && n < this->num);
    return this->data[n*this->num_fields + field];
}

void JIntList::il_set(int n, int field, int val)
{
    assert(n >= 0 && n < this->num);
    this->data[n*this->num_fields + field] = val;
}

int JIntList::il_push_back()
{
    const int new_pos = (this->num+1) * this->num_fields;

    // If the list is full, we need to reallocate the buffer to make room
    // for the new element.
    if (new_pos > this->cap)
    {
        // Use double the size for the new capacity.
        const int new_cap = new_pos * 2;

        // If we're pointing to the fixed buffer, allocate a new array on the
        // heap and copy the fixed buffer contents to it.
        if (this->cap == il_fixed_cap)
        {
            this->data = (int*) malloc(new_cap * sizeof(*this->data));
            memcpy(this->data, this->fixed, sizeof(this->fixed));
        }
        else
        {
            // Otherwise reallocate the heap buffer to the new size.
            this->data = (int*) realloc(this->data, new_cap * sizeof(*this->data));
        }
        // Set the old capacity to the new capacity.
        this->cap = new_cap;
    }
    return this->num++;
}

void JIntList::il_pop_back()
{
    // Just decrement the list size.
    assert(this->num > 0);
    --this->num;
}

int JIntList::il_insert()
{
    // If there's a free index in the free list, pop that and use it.
    if (this->free_element != -1)
    {
        const int index = this->free_element;
        const int pos = index * this->num_fields;

        // Set the free index to the next free index.
        this->free_element = this->data[pos];

        // Return the free index.
        return index;
    }
    // Otherwise insert to the back of the array.
    return il_push_back();
}

void JIntList::il_erase(int n)
{
    // Push the element to the free list.
    const int pos = n * this->num_fields;
    this->data[pos] = this->free_element;
    this->free_element = n;
}