#pragma once
#include <cstdint>
#include <chrono>
#include <functional>
#include <SDL_render.h>

#include "jmath.h"
#include "jint_list.h"

using namespace std;

struct QuadRect
{
    int midX;
    int midY;
    int halfW;
    int halfH;

    QuadRect() : midX(0), midY(0), halfW(0), halfH(0) {}
    QuadRect(int mid_x, int mid_y, int half_w, int half_h)
        : midX(mid_x), midY(mid_y), halfW(half_w), halfH(half_h) {}

    inline int W4() { return halfW >> 1; }
    inline int H4() { return halfH >> 1; }

    inline QuadRect TL() { return (QuadRect(midX - W4(), midY + H4(), W4(), H4())); }
    inline QuadRect TR() { return (QuadRect(midX + W4(), midY + H4(), W4(), H4())); }
    inline QuadRect BL() { return (QuadRect(midX - W4(), midY - H4(), W4(), H4())); }
    inline QuadRect BR() { return (QuadRect(midX + W4(), midY - H4(), W4(), H4())); }

    Rect ToRect()
    {
        return Rect(midX, midY, halfW << 1, halfH << 1);
    }

    SDL_Rect ToSDL(Mat3 &transform)
    {
        Vec2 p = transform * Vec2(midX - halfW, midY + halfH);
        SDL_Rect rect;
        rect.x = (int)p.x;
        rect.y = (int)p.y;
        rect.w = halfW << 1;
        rect.h = halfW << 1;
        return rect;
    }
};

class QuadTree
{
public:
    const static int ROOT_QUAD_NODE_INDEX = 0;
    using QueryCallback = void(
        void *user_data,
        QuadTree *tree,
        int nodeIndex,
        Rect nodeRect,
        int depth);

    // Origin is center, x+ is right, y+ is up
    QuadRect _Bounds;
    QuadElementIntList _Elements;
    QuadElementNodeIntList _ElementNodes;
    QuadNodesIntList _Nodes;

private:
    int _splitThreshold = 3;
    int _maxDepth = 25;

public:
    QuadTree(Rect bounds, int maxDepth, int splitThreshold);
    ~QuadTree();

    int Insert(int id, Rect &rect);
    void Remove(int elementIndex);

    // Returns list of elements which intersect the query rectangle
    void Query(Rect query,
               unordered_map<int, bool> &seenElements,
               vector<int> *output);

    // Traverse through every branch/leaf node in the quad tree
    // Invokes the callbacks in the order of traversal
    void Traverse(void *userData,
                  QueryCallback branchCallback,
                  QueryCallback leafCallback);

    void Clean();

    void Draw(SDL_Renderer *renderer,
              Mat3 &transform,
              chrono::milliseconds deltaMs,
              bool renderRects);

protected:
    bool Intersects(int aLeft, int aTop, int aRight, int aBottom,
                    int bLeft, int bTop, int bRight, int bBottom);

    void FindLeavesList(int quadNodeIndex,
                        int midX, int midY, int halfW, int halfH,
                        int depth,
                        int left, int top, int right, int bottom,
                        LeavesListIntList &output);
                        
    void InsertNode(int quadNodeIndex,
                    int midX, int midY, int halfW, int halfH,
                    int depth,
                    int elementIndex);
                    
    void InsertLeafNode(int quadNodeIndex,
                        int midX, int midY, int halfW, int halfH,
                        int depth,
                        int elementIndex);
};
