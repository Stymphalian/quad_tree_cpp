#pragma once
#include "jmath.h"

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



class QuadNodesIntList : public JIntList
{
public:
    enum
    {
        children = 0,
        count = 1
    };
    QuadNodesIntList() : JIntList(2) {}

    int AddLeaf()
    {
        int id = il_insert();
        data[id * num_fields + children] = -1;
        data[id * num_fields + count] = 0;
        return id;
    }

    void MakeBranch(int id, int childrenId)
    {
        data[id * num_fields + children] = childrenId;
        data[id * num_fields + count] = -1;
    }

    bool IsLeaf(int id)
    {
        return data[id * num_fields + count] >= 0;
    }

    bool IsBranch(int id)
    {
        return data[id * num_fields + count] == -1;
    }

    bool IsEmpty(int id)
    {
        return data[id * num_fields + count] == 0;
    }

    int GetChildren(int id)
    {
        return data[id * num_fields + children];
    }
    int GetCount(int id)
    {
        return data[id * num_fields + count];
    }
    void SetChildren(int id, int v)
    {
        data[id * num_fields + children] = v;
    }
    void SetCount(int id, int v)
    {
        data[id * num_fields + count] = v;
    }
};

class QuadElementIntList : public JIntList
{
public:
    enum
    {
        ID = 0,
        left = 1,
        top = 2,
        right = 3,
        bottom = 4
    };
    QuadElementIntList() : JIntList(5) {}

    int Add(int tId, int l, int t, int r, int b)
    {
        int id = il_insert();
        data[id * num_fields + ID] = tId;
        data[id * num_fields + left] = l;
        data[id * num_fields + top] = t;
        data[id * num_fields + right] = r;
        data[id * num_fields + bottom] = b;
        return id;
    }

    int GetId(int id)
    {
        return data[id * num_fields + ID];
    }
    void SetId(int id, int val)
    {
        data[id * num_fields + ID] = val;
    }

    Rect GetRect(int id)
    {
        const int l = data[id * num_fields + left];
        const int t = data[id * num_fields + top];
        const int r = data[id * num_fields + right];
        const int b = data[id * num_fields + bottom];
        return Rect(l, t, abs(r - l), abs(t - b));
    }

    int GetLeft(int id)
    {
        return data[id * num_fields + left];
    }
    int GetTop(int id)
    {
        return data[id * num_fields + top];
    }
    int GetRight(int id)
    {
        return data[id * num_fields + right];
    }
    int GetBottom(int id)
    {
        return data[id * num_fields + bottom];
    }
    void SetLeft(int id, int val)
    {
        data[id * num_fields + left] = val;
    }
    void SetTop(int id, int val)
    {
        data[id * num_fields + top] = val;
    }
    void SetRight(int id, int val)
    {
        data[id * num_fields + right] = val;
    }
    void SetBottom(int id, int val)
    {
        data[id * num_fields + bottom] = val;
    }
};

class QuadElementNodeIntList : public JIntList
{
public:
    enum
    {
        next = 0,
        elementId = 1
    };
    QuadElementNodeIntList() : JIntList(2) {}

    int Add(int elementIndex)
    {
        int id = il_insert();
        data[id * num_fields + next] = -1;
        data[id * num_fields + elementId] = elementIndex;
        return id;
    }

    int GetNext(int id)
    {
        return data[id * num_fields + next];
    }
    void SetNext(int id, int val)
    {
        data[id * num_fields + next] = val;
    }

    int GetElementId(int id)
    {
        return data[id * num_fields + elementId];
    }
    void SetElementId(int id, int val)
    {
        data[id * num_fields + elementId] = val;
    }
};

class LeavesListIntList : public JIntList
{
public:
    enum
    {
        nd_mx = 0,
        nd_my = 1,
        nd_sx = 2,
        nd_sy = 3,
        nd_index = 4,
        nd_depth = 5,
    };
    LeavesListIntList() : JIntList(6) {}

    int Add(int nd_index1, int nd_depth1, int nd_mx1, int nd_my1, int nd_sx1, int nd_sy1)
    {
        const int back_idx = il_push_back();
        data[back_idx * num_fields + nd_mx] = nd_mx1;
        data[back_idx * num_fields + nd_my] = nd_my1;
        data[back_idx * num_fields + nd_sx] = nd_sx1;
        data[back_idx * num_fields + nd_sy] = nd_sy1;
        data[back_idx * num_fields + nd_index] = nd_index1;
        data[back_idx * num_fields + nd_depth] = nd_depth1;
        return back_idx;
    }

    int GetMx(int id)
    {
        return data[id * num_fields + nd_mx];
    }

    int GetMy(int id)
    {
        return data[id * num_fields + nd_my];
    }

    int GetSx(int id)
    {
        return data[id * num_fields + nd_sx];
    }

    int GetSy(int id)
    {
        return data[id * num_fields + nd_sy];
    }

    int GetIndex(int id)
    {
        return data[id * num_fields + nd_index];
    }

    int GetDepth(int id)
    {
        return data[id * num_fields + nd_depth];
    }
};


