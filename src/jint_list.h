#pragma once


// typedef struct IntList IntList;
enum {il_fixed_cap = 128};
class JIntList
{
public:
    // Stores a fixed-size buffer in advance to avoid requiring
    // a heap allocation until we run out of space.
    int fixed[il_fixed_cap];

    // Points to the buffer used by the list. Initially this will
    // point to 'fixed'.
    int* data;

    // Stores how many integer fields each element has.
    int num_fields;

    // Stores the number of elements in the list.
    int num;

    // Stores the capacity of the array.
    int cap;

    // Stores an index to the free element or -1 if the free list
    // is empty.
    int free_element;

    // ---------------------------------------------------------------------------------
    // List Interface
    // ---------------------------------------------------------------------------------
    // Creates a new list of elements which each consist of integer fields.
    // 'num_fields' specifies the number of integer fields each element has.
    // void il_create(int num_fields);
    // // Destroys the specified list.
    // void il_destroy();

    JIntList(int num_fields);
    ~JIntList();

    // Returns the number of elements in the list.
    int il_size();

    // Returns the value of the specified field for the nth element.
    int il_get(int n, int field);

    // Sets the value of the specified field for the nth element.
    void il_set(int n, int field, int val);

    // Clears the specified list, making it empty.
    void il_clear();


    // ---------------------------------------------------------------------------------
    // Stack Interface (do not mix with free list usage; use one or the other)
    // ---------------------------------------------------------------------------------
    // Inserts an element to the back of the list and returns an index to it.
    int il_push_back();

    // Removes the element at the back of the list.
    void il_pop_back();

    // ---------------------------------------------------------------------------------
    // Free List Interface (do not mix with stack usage; use one or the other)
    // ---------------------------------------------------------------------------------
    // Inserts an element to a vacant position in the list and returns an index to it.
    int il_insert();

    // Removes the nth element in the list.
    void il_erase(int n);
};





