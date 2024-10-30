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

    Rect ToRect() {
        return Rect(mid_x , mid_y, half_w << 1, half_h << 1);
    }

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


typedef void QtNodeFunc(void* user_data, int node, int depth, int mx, int my, int sx, int sy);

class QuadTree
{
public:
    const static int ROOT_QUAD_NODE_INDEX = 0;
    using QueryCallback = std::function<void(void* user_data, QuadTree* tree, int nodeIndex, Rect nodeRect, int depth)>;

    QuadElementIntList Elements;
    QuadElementNodeIntList ElementNodes;
    QuadNodesIntList Nodes;

    // Origin is center, x+ is right, y+ is up
    QuadRect Bounds;
    int split_threshold = 3;
    int max_depth = 25;

    QuadTree(Rect bounds, int max_depth, int split_threshold);
    ~QuadTree();

    int Insert(int id, Rect &rect);
    void Remove(int elementIndex);

    // Returns list of elements which intersect the query rectangle
    void Query(Rect query, unordered_map<int, bool>& seenElements, vector<int>* output);

    // Traverse through every branch/leaf node in the quad tree
    // Invokes the callbacks in the order of traversal
    void Traverse(void* userData, QueryCallback branchCallback, QueryCallback leafCallback);

    void Clean();

    void Draw(
        SDL_Renderer *renderer,
        Mat3 &transform,
        chrono::milliseconds delta_ms,
        bool render_rects
    );
    

protected:
    void FindLeavesList(
        int quadNodeIndex,
        int mid_x, int mid_y, int half_w, int half_h,
        int depth,
        int left, int top, int right, int bottom,
        LeavesListIntList &output);
    void InsertNode(
        int quadNodeIndex,
        int mid_x, int mid_y, int half_w, int half_h,
        int depth,
        int elementIndex);
    void InsertLeafNode(
        int quadNodeIndex,
        int mid_x, int mid_y, int half_w, int half_h,
        int depth,
        int elementIndex);
};
