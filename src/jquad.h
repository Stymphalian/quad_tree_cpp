#pragma once
#include <cstdint>
#include "jmath.h"
#include "free_list.hpp"
#include <functional>
#include <chrono>
#include <SDL_render.h>

#include "metrics.h"
#include "jint_list.h"

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

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

struct QuadRect
{
    int mid_x;
    int mid_y;
    int half_w;
    int half_h;
    QuadRect() : mid_x(0), mid_y(0), half_w(0), half_h(0) {}
    QuadRect(int mid_x, int mid_y, int half_w, int half_h) : mid_x(mid_x), mid_y(mid_y), half_w(half_w), half_h(half_h) {}

    inline int W4() { return half_w >> 1; }
    inline int H4() { return half_h >> 1; }

    inline QuadRect TL() { return QuadRect(mid_x - W4(), mid_y + H4(), W4(), H4()); }
    inline QuadRect TR() { return QuadRect(mid_x + W4(), mid_y + H4(), W4(), H4()); }
    inline QuadRect BL() { return QuadRect(mid_x - W4(), mid_y - H4(), W4(), H4()); }
    inline QuadRect BR() { return QuadRect(mid_x + W4(), mid_y - H4(), W4(), H4()); }

    SDL_Rect ToSDL(Mat3 &transform)
    {
        Vec2 p = transform * Vec2(mid_x - half_w, mid_y + half_h);
        SDL_Rect rect;
        rect.x = (int)p.x;
        rect.y = (int)p.y;
        rect.w = half_w << 1;
        rect.h = half_w << 1;
        return rect;
    }
};

class QuadTree
{
public:
    const static int ROOT_QUAD_NODE_INDEX = 0;

    // Origin is center, x+ is right, y+ is up
    QuadElementIntList Elements;
    QuadElementNodeIntList ElementNodes;
    QuadNodesIntList Nodes;

    // Rect Bounds;
    QuadRect Bounds;
    int split_threshold = 3;
    int max_depth = 25;

    // using LeafCallbackFn = std::function<void(int quadNodeIndex, QuadRect nodeRect, int depth)>;
    // using LeafCallbackFn = std::function<void(int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth)>;
    // using LeafCallbackWithTreeFn = std::function<void(QuadTree *tree, int quadNodeIndex, QuadRect nodeRect, int depth)>;
    // typedef void QuadTreeCallback(
    //     QuadTree *tree,
    //     void *user_data,
    //     int quadNodeIndex,
    //     Rect &nodeRect,
    //     int depth);

    // QuadTree();
    QuadTree(Rect bounds, int max_depth, int split_threshold);
    ~QuadTree();

    int Insert(int id, Rect &rect);
    void Remove(int elementIndex);

    // void Query(Rect &queryRect, LeafCallbackWithTreeFn callback);

    // template <typename T>
    // void QueryWithData(
    //     Rect &queryRect,
    //     std::function<T *(int id)> getDataFn,
    //     std::function<void(T *data)> useDataFn);

    // void QueryWithCallback(Rect &queryRect, void *user_data, QuadTreeCallback *callback);

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms);

    void Clean();

protected:
    struct FindLeavesListData
    {
        int quadNodeIndex;
        QuadRect nodeRect;
        int depth;
    };

    void FindLeavesList(
        int quadNodeIndex,
        int mid_x, int mid_y, int half_w, int half_h,
        int depth,
        int left, int top, int right, int bottom,
        // int elementIndex,
        LeavesListIntList &output);
    // void FindLeaves(int quadNodeIndex,
    //                 // QuadRect quadNodeRect,
    //                 int mid_x, int mid_y, int half_w, int half_h,
    //                 int depth,
    //                 int elementIndex,
    //                 LeafCallbackFn leafCallbackFn);

    void InsertNode(int quadNodeIndex, 
        int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex);
    void InsertLeafNode(int quadNodeIndex,
         int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex);
    // void InsertNode(int quadNodeIndex, QuadRect nodeRect, int depth, int elementIndex);
    // void InsertLeafNode(int quadNodeIndex, QuadRect nodeRect, int depth, int elementIndex);
    // bool ShouldSplitNode();
};
