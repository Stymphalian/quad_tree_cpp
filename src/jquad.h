#pragma once
#include <cstdint>
#include "jmath.h"
#include "free_list.hpp"
#include <functional>
#include <chrono>

#include "metrics.h"
#include <SDL_render.h>

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

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

struct QuadNode
{
    // If a branch node then the points to the first child QuadNode,
    // else it points to the first Element of the leaf
    int32_t children;
    // Number of children in the leaf. -1 if a branch node.
    int32_t count;

    inline bool IsBranch()
    {
        return (count == -1);
    }
    inline bool IsLeaf()
    {
        return (count >= 0);
    }

    void MakeBranch(int trIndex)
    {
        children = trIndex;
        count = -1;
    }

    void MakeLeaf()
    {
        children = -1;
        count = 0;
    }

    inline bool HasChildren()
    {
        return (count > 0);
    }
    inline bool IsEmpty()
    {
        return (count == 0);
    }

    static QuadNode Branch(int children = -1)
    {
        QuadNode node;
        node.count = -1;
        node.children = children;
        return node;
    }
    static QuadNode Leaf(int children = -1)
    {
        QuadNode node;
        node.count = 0;
        node.children = -1;
        return node;
    }

    QuadNode() : children(-1), count(-1) {}
    inline int TL() { return (children + 0); }
    inline int TR() { return (children + 1); }
    inline int BL() { return (children + 2); }
    inline int BR() { return (children + 3); }
};

// Represents an element in the quadtree.
struct QuadElement
{
    // Stores the ID for the element (can be used to
    // refer to external data).
    int id;

    // Stores the rectangle for the element.
    Rect rect;
    // int x1, y1, x2, y2;

    QuadElement(int id, Rect rect) : id(id), rect(rect) {}
};

// Represents an element node in the quadtree.
struct QuadElementNode
{
    // Points to the next element in the leaf node. A value of -1
    // indicates the end of the list.
    int next;

    // Stores the element index.
    int element;

    QuadElementNode(int elementIndex) : next(-1), element(elementIndex) {}

    inline void MakeEmpty()
    {
        (this->next = -1);
    }
};

class QuadTree
{
public:
    const static int ROOT_QUAD_NODE_INDEX = 0;

    // Origin is center, x+ is right, y+ is up
    FreeList<QuadElement> Elements;
    FreeList<QuadElementNode> ElementNodes;
    FreeList<QuadNode> Nodes;
    // Rect Bounds;
    QuadRect Bounds;
    int split_threshold = 3;
    int max_depth = 25;

    using LeafCallbackFn = std::function<void(int quadNodeIndex, QuadRect nodeRect, int depth)>;
    using LeafCallbackWithTreeFn = std::function<void(QuadTree *tree, int quadNodeIndex, QuadRect nodeRect, int depth)>;
    typedef void QuadTreeCallback(
        QuadTree *tree,
        void *user_data,
        int quadNodeIndex,
        Rect &nodeRect,
        int depth);

    QuadTree();
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
    void InsertNode(int quadNodeIndex, QuadRect nodeRect, int depth, int elementIndex);
    void FindLeaves(int quadNodeIndex,
                    QuadRect quadNodeRect,
                    int depth,
                    Rect &target,
                    LeafCallbackFn leafCallbackFn);
    void InsertLeafNode(int quadNodeIndex, QuadRect nodeRect, int depth, int elementIndex);
    bool ShouldSplitNode(QuadNode &node);
};
