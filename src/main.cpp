
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <chrono>
#include <iostream>
#include <tuple>
#include <cassert>

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "bitmap_image.hpp"
#include "free_list.hpp"
#include "jmath.h"
#include "IntList.h"
#include "quad_tree.h"

// 13 FPS
const int WORLD_WIDTH = 10000;
const int WORLD_HEIGHT = 10000;
const int NUMBER_SPRITES = 20000;
const int MAX_SPRITE_VELOCITY = 120;
const int MIN_RECT_SIZE = 1;
const int MAX_RECT_SIZE = 50;
const int MAX_QUAD_TREE_DEPTH = 16;
const int QUAD_TREE_SPLIT_THRESHOLD = 8;
const bool USE_QUAD_TREE = true;
const int VIEWPORT_WIDTH = 1920;
const int VIEWPORT_HEIGHT = 960;

// const int WORLD_WIDTH = 200;
// const int WORLD_HEIGHT = 200;
// const int NUMBER_SPRITES = 50;
// const int MAX_SPRITE_VELOCITY = 120;
// const int MIN_RECT_SIZE = 5;
// const int MAX_RECT_SIZE = 10;
// const int MAX_QUAD_TREE_DEPTH = 8;
// const int QUAD_TREE_SPLIT_THRESHOLD = 3;
// const bool USE_QUAD_TREE = true;
// const int VIEWPORT_WIDTH = 400;
// const int VIEWPORT_HEIGHT = 400;

int current_frame = 0;

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

class Metrics
{
public:
    unordered_map<string, std::chrono::nanoseconds> metrics;

    void Add(string name, std::chrono::nanoseconds value)
    {
        if (metrics.find(name) == metrics.end())
        {
            metrics[name] = value;
        }
        else
        {
            metrics[name] += value;
        }
    }

    void Print()
    {
        // std::chrono::nanoseconds total_time = metrics.at("total_time");
        auto total_time = chrono::duration_cast<chrono::milliseconds>(metrics.at("total_time")).count();

        vector<pair<string, std::chrono::nanoseconds>> ordered_metrics(metrics.begin(), metrics.end());
        sort(ordered_metrics.begin(), ordered_metrics.end(), [](const pair<string, std::chrono::nanoseconds> &a, const pair<string, std::chrono::nanoseconds> &b)
             { return a.first < b.first; });

        // for (auto it = metrics.begin(); it != metrics.end(); it++) {
        for (auto &entry : ordered_metrics)
        {
            auto name = entry.first;
            auto val = entry.second;
            auto ms = chrono::duration_cast<chrono::milliseconds>(val).count();
            printf("%s: %lld msec (%.2f%%)\n",
                   name.c_str(),
                   ms,
                   100.0f * ms / total_time);
        }
    }

    void RecordSpriteUpdatePhyscis(std::chrono::nanoseconds delta)
    {
        Add("sprite_update_physics_usec", delta);
    }
    void RecordSpriteQuadTreeUpdate(std::chrono::nanoseconds delta)
    {
        Add("sprite_quad_tree_update_usec", delta);
    }
    void RecordSpriteCollide(std::chrono::nanoseconds delta)
    {
        Add("sprite_collide_usec", delta);
    }
    void RecordQuadCollision(std::chrono::nanoseconds delta)
    {
        Add("quad_collision_usec", delta);
    }
    void RecordQuadTreeInsert(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert", delta);
    }
    void RecordQuadTreeRemove(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_remove", delta);
    }
    void RecordQuadClean(std::chrono::nanoseconds delta)
    {
        Add("quad_clean", delta);
    }
    void RecordSceneDraw(std::chrono::nanoseconds delta)
    {
        Add("scene_draw", delta);
    }
    void RecordTotalTime(std::chrono::nanoseconds delta)
    {
        Add("total_time", delta);
    }
    void RecordQuadTreeRemoveInner(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_remove_inner", delta);
    }
    void RecordQuadFindLeaves(std::chrono::nanoseconds delta)
    {
        Add("quad_find_leaves", delta);
    }
    void RecordQuadTreeInsertNode(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert_node", delta);
    }
    void RecordQuadInsertLeafNode(std::chrono::nanoseconds delta)
    {
        Add("quad_tree_insert_leaf_node", delta);
    }
};
Metrics METRICS;

class Rect
{
public:
    // x,y is center of rect, w,h is full width and height
    int x;
    int y;
    int w;
    int h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
    ~Rect() = default;

    Vec2 Center() { return Vec2(x, y); }

    inline int L() const { return (x - (w >> 1)); }
    inline int R() const { return (x + (w >> 1)); }
    inline int T() const { return (y + (h >> 1)); }
    inline int B() const { return (y - (h >> 1)); }
    inline int Ox() const { return (x); }
    inline int Oy() const { return (y); }
    inline int W2() const { return (w >> 1); }
    inline int H2() const { return (h >> 1); }
    inline int W4() const { return (w >> 2); }
    inline int H4() const { return (h >> 2); }
    inline Rect TR() { return Rect(x + W4(), y + H4(), W2(), H2()); }
    inline Rect TL() { return Rect(x - W4(), y + H4(), W2(), H2()); }
    inline Rect BL() { return Rect(x - W4(), y - H4(), W2(), H2()); }
    inline Rect BR() { return Rect(x + W4(), y - H4(), W2(), H2()); }

    bool Intersects(Rect r)
    {
        return (
            r.L() < R() &&
            r.R() > L() &&
            r.T() > B() &&
            r.B() < T());
    }

    SDL_Rect ToSDL(Mat3 &transform)
    {
        Vec2 p = transform * Vec2(L(), T());
        SDL_Rect rect;
        rect.x = (int)p.x;
        rect.y = (int)p.y;
        rect.w = w;
        rect.h = h;
        return rect;
    }
};

struct PhysicsUpdate
{
    Vec2 Velocity = Vec2(0, 0);
    Vec2 Drift = Vec2(0, 0);
    bool HasUpdate = false;
};

class Sprite
{
public:
    Vec2 Position;
    Vec2 Velocity;
    Rect BoundingBox;
    double Mass = 1.0;
    int Id;
    int QuadId;
    bool IsColliding = false;
    unordered_map<int, bool> already_collided;

    PhysicsUpdate update;

    Sprite(int Id, Vec2 position, Vec2 velocity, Rect BB)
    {
        this->Position = position;
        this->Velocity = velocity;
        this->BoundingBox = BB;
        this->Id = Id;
        this->QuadId = -1;
        this->update = PhysicsUpdate();
    };
    ~Sprite() = default;

    void Update(Rect &bounds, chrono::milliseconds delta)
    {
        already_collided.clear();
        if (this->update.HasUpdate)
        {
            this->Position -= this->update.Drift;
            this->Velocity = this->update.Velocity;
            this->update.HasUpdate = false;
        }
        else
        {
            this->update.Velocity = this->Velocity;
            this->update.Drift.ZeroOut();
            this->update.HasUpdate = false;
        }

        this->Velocity.Clamp(-MAX_SPRITE_VELOCITY, MAX_SPRITE_VELOCITY);
        this->Position += (this->Velocity * (float)(delta.count() / 1000.0f));

        if (this->Position.x < bounds.L())
        {
            this->Position.x = (float)bounds.L();
            this->Velocity.x = -this->Velocity.x;
        }
        else if (this->Position.x > bounds.R())
        {
            this->Position.x = (float)bounds.R();
            this->Velocity.x = -this->Velocity.x;
        }
        if (this->Position.y < bounds.B())
        {
            this->Position.y = (float)bounds.B();
            this->Velocity.y = -this->Velocity.y;
        }
        else if (this->Position.y > bounds.T())
        {
            this->Position.y = (float)bounds.T();
            this->Velocity.y = -this->Velocity.y;
        }

        this->BoundingBox.x = (int)this->Position.x;
        this->BoundingBox.y = (int)this->Position.y;
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        Vec2 newPos = transform * this->Position;
        SDL_Rect rect = {
            (int)newPos.x - this->BoundingBox.W2(),
            (int)newPos.y - this->BoundingBox.H2(),
            this->BoundingBox.w,
            this->BoundingBox.h};

        if (this->IsColliding)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        }
        SDL_RenderDrawPoint(renderer, (int)newPos.x, (int)newPos.y);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool Intersects(Sprite *other)
    {
        return (this->Position - other->Position).Length() < (this->BoundingBox.w + other->BoundingBox.w) / 2;
        // return this->BoundingBox.Intersects(other->BoundingBox);
    }

    void Collides(Sprite *B)
    {
        auto start_time = rclock::now();
        Sprite *A = this;
        Vec2 dir = (B->Position - A->Position).Normalize(); // A to B
        Vec2 rdir = dir * -1.0;
        Vec2 dir_norm = dir.GetNormal();
        Vec2 rdir_norm = dir * -1.0;

        Vec2 comp_dir_a = dir * A->Velocity.dot(dir);
        Vec2 comp_dir_b = rdir * B->Velocity.dot(rdir);
        Vec2 comp_norm_a = dir_norm * A->Velocity.dot(dir_norm);
        Vec2 comp_norm_b = rdir_norm * B->Velocity.dot(rdir_norm);

        A->Velocity = comp_norm_a + comp_dir_b;
        B->Velocity = comp_norm_b + comp_dir_a;
        A->Position = A->Position - dir;
        B->Position = B->Position - rdir;
        auto end_time = rclock::now();
        METRICS.RecordSpriteCollide(end_time - start_time);

        // auto start_time = rclock::now();
        // Sprite *A = this;
        // Vec2 dir = (B->Position - A->Position).Normalize();  // A to B
        // Vec2 rdir = (A->Position - B->Position).Normalize(); // B to A
        // Vec2 dir_norm = dir.GetNormal();
        // Vec2 rdir_norm = rdir.GetNormal();

        // Vec2 comp_dir_a = dir * A->Velocity.dot(dir);
        // Vec2 comp_dir_b = rdir * B->Velocity.dot(rdir);
        // Vec2 comp_norm_a = dir_norm * A->Velocity.dot(dir_norm);
        // Vec2 comp_norm_b = rdir_norm * B->Velocity.dot(rdir_norm);

        // A->Velocity = comp_norm_a + comp_dir_b;
        // B->Velocity = comp_norm_b + comp_dir_a;
        // A->Position = A->Position - dir;
        // B->Position = B->Position - rdir;
        // auto end_time = rclock::now();
        // METRICS.RecordSpriteCollide(end_time - start_time);

        // double iM1M2 = 1.0 / (A->Mass + B->Mass);
        // Vec2 newA, newAPos;
        // Vec2 newB, newBPos;
        // vector<tuple<Sprite*, Sprite*, Vec2*, Vec2*>> things = {
        //     {A, B, &newA, &newAPos},
        //     {B, A , &newB, &newBPos}
        // };
        // for (auto t: things) {
        //     Sprite* A = std::get<0>(t);
        //     Sprite* B = std::get<1>(t);
        //     Vec2* into = std::get<2>(t);
        //     Vec2* intoPos = std::get<3>(t);

        //     Vec2 v_diff = A->Velocity - B->Velocity;
        //     Vec2 p_diff = A->Position - B->Position;
        //     Vec2 dir = p_diff.Normalize();

        //     double mass = 2 * B->Mass * iM1M2;
        //     double factor = v_diff.dot(p_diff) / p_diff.magnitudeSquared();
        //     Vec2 temp =  p_diff * (mass * factor);
        //     *into = A->Velocity + temp;
        //     *intoPos = p_diff.Normalize();
        // }

        // A->Velocity = newA;
        // A->Position -= newAPos;
        // B->Velocity = newB;
        // B->Position -= newBPos;
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
    inline int TR() { return children + 0; }
    inline int TL() { return children + 1; }
    inline int BL() { return children + 2; }
    inline int BR() { return children + 3; }
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

void QuadHandleBranch(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy);
void QuadHandleLeaf(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy);
void QuadHandleDrawBranch(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy);
void QuadHandleDrawLeaf(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy);

class QuadTree
{
public:
    const static int ROOT_QUAD_NODE_INDEX = 0;

    // Origin is center, x+ is right, y+ is up
    FreeList<QuadElement> Elements;
    FreeList<QuadElementNode> ElementNodes;
    FreeList<QuadNode> Nodes;
    Rect Bounds;
    int split_threshold = 3;
    int max_depth = 25;

    using LeafCallbackFn = std::function<void(int quadNodeIndex, Rect &nodeRect, int depth)>;
    using LeafCallbackWithTreeFn = std::function<void(QuadTree *tree, int quadNodeIndex, Rect &nodeRect, int depth)>;

    QuadTree() : max_depth(3), Bounds(Rect(0, 0, 100, 100)), split_threshold(3)
    {
        Nodes.insert(QuadNode::Leaf());
    }
    QuadTree(Rect bounds, int max_depth, int split_threshold) : Bounds(bounds), max_depth(max_depth), split_threshold(split_threshold)
    {
        Nodes.insert(QuadNode::Leaf());
    };
    ~QuadTree()
    {
    }

    int Insert(int id, Rect &rect)
    {
        auto start_time = rclock::now();
        int elementIndex = Elements.insert(QuadElement(id, rect));
        InsertNode(0, Bounds, 0, elementIndex);
        auto end_time = rclock::now();
        METRICS.RecordQuadTreeInsert(end_time - start_time);

        return elementIndex;
    }

    void Remove(int elementIndex)
    {
        // int rootNodeIndex = 0;
        auto start_time = rclock::now();
        int depth = 0;
        FindLeaves(
            ROOT_QUAD_NODE_INDEX,
            Bounds,
            depth,
            Elements[elementIndex].rect,
            [this, elementIndex](int quadNodeIndex, Rect nodeRect, int depth)
            {
                auto start_remove_inner = rclock::now();
                QuadNode *node = &(Nodes[quadNodeIndex]);
                node->count -= 1;

                int beforeIndex = -1;
                int currentIndex = node->children;
                QuadElementNode *current = nullptr;
                while (currentIndex != -1)
                {
                    current = &(ElementNodes[currentIndex]);
                    if (current->element == elementIndex)
                    {
                        break;
                    }
                    beforeIndex = currentIndex;
                    currentIndex = current->next;
                }

                if (beforeIndex == -1)
                {
                    node->children = current->next;
                }
                else
                {
                    QuadElementNode *before = &(ElementNodes[beforeIndex]);
                    before->next = current->next;
                }
                ElementNodes.erase(currentIndex);

                auto end_remove_inner = rclock::now();
                METRICS.RecordQuadTreeRemoveInner(end_remove_inner - start_remove_inner);
            });
        Elements.erase(elementIndex);
        auto end_time = rclock::now();
        METRICS.RecordQuadTreeRemove(end_time - start_time);
    }

    void Query(Rect &targetRect, LeafCallbackWithTreeFn callback)
    {
        int depth = 0;
        FindLeaves(
            ROOT_QUAD_NODE_INDEX,
            Bounds,
            depth,
            targetRect,
            [this, callback](int quadNodeIndex, Rect nodeRect, int depth)
            {
                callback(this, quadNodeIndex, nodeRect, depth);
            });
    }

    template <typename T>
    void QueryWithData(
        Rect &targetRect,
        std::function<T *(int id)> getDataFn,
        std::function<void(T *data)> useDataFn)
    {
        int depth = 0;
        FindLeaves(
            ROOT_QUAD_NODE_INDEX,
            Bounds,
            depth,
            targetRect,
            [this, getDataFn, useDataFn](int quadNodeIndex, Rect nodeRect, int depth)
            {
                QuadNode &node = Nodes[quadNodeIndex];

                int elementNodeIndex = node.children;
                while (elementNodeIndex != -1)
                {
                    QuadElementNode &en = ElementNodes[elementNodeIndex];
                    QuadElement &element = Elements[en.element];
                    T *data = getDataFn(element.id);
                    useDataFn(data);
                    elementNodeIndex = en.next;
                }
            });
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        // TODO: Draw the quadtree
        vector<tuple<QuadNode *, Rect, int>> nodes;
        nodes.push_back(make_tuple(&(this->Nodes[0]), this->Bounds, 0));
        while (nodes.size() > 0)
        {
            auto [current, currentRect, depth] = nodes.back();
            nodes.pop_back();

            if (current->IsLeaf())
            {
                if (depth >= 0)
                {
                    int color = 64 + (128 - depth * (128 / max_depth));
                    SDL_Rect rect = currentRect.ToSDL(transform);
                    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
                    SDL_RenderDrawRect(renderer, &rect);
                }
            }
            else if (current->IsBranch())
            {
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TL()]),
                    currentRect.TL(),
                    depth + 1));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TR()]),
                    currentRect.TR(),
                    depth + 1));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BL()]),
                    currentRect.BL(),
                    depth + 1));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BR()]),
                    currentRect.BR(),
                    depth + 1));
            }
        }
    }

    void Clean()
    {
        if (Nodes[0].IsLeaf())
        {
            return;
        }
        auto start_time = rclock::now();

        vector<int> to_process;
        to_process.push_back(0);
        while (to_process.size() > 0)
        {
            int currentIndex = to_process.back();
            to_process.pop_back();
            QuadNode &current = Nodes[currentIndex];

            // Loop through all the children.
            // count the number of leaves and insert any branch nodes to process
            bool all_leaves = true;
            for (int i = current.TR(); i <= current.BR(); i++)
            {
                QuadNode &childNode = Nodes[i];
                if (childNode.IsBranch())
                {
                    all_leaves = false;
                    to_process.push_back(i);
                }
                else
                {
                    all_leaves &= childNode.IsEmpty();
                }
            }

            // If all the child nodes are leaves we can delete this branch
            // node and make it a leaf node again.
            if (all_leaves)
            {
                Nodes.erase(current.BR());
                Nodes.erase(current.BL());
                Nodes.erase(current.TL());
                Nodes.erase(current.TR());

                Nodes.erase(currentIndex);
                Nodes.insert(QuadNode::Leaf());
            }
        }
        auto end_time = rclock::now();
        METRICS.RecordQuadClean(end_time - start_time);
    }

    struct LeafIterData
    {
        int nodeIndex;
        Rect rect;
        int depth;
    };
    void GetAllLeaves(vector<LeafIterData> *output)
    {
        vector<tuple<int, Rect, int>> stack;
        stack.push_back(make_tuple(0, Bounds, 0));

        QuadNode *current = nullptr;
        while (stack.size() > 0)
        {
            auto [currentIndex, rect, depth] = stack.back();
            current = &(Nodes[currentIndex]);
            stack.pop_back();

            if (current->IsLeaf())
            {
                output->push_back(LeafIterData{currentIndex, rect, depth});
            }
            else
            {
                stack.push_back(make_tuple(current->TR(), rect.TR(), depth + 1));
                stack.push_back(make_tuple(current->TL(), rect.TL(), depth + 1));
                stack.push_back(make_tuple(current->BL(), rect.BL(), depth + 1));
                stack.push_back(make_tuple(current->BR(), rect.BR(), depth + 1));
            }
        }
    }

protected:
    void InsertNode(int quadNodeIndex, Rect &nodeRect, int depth, int elementIndex)
    {
        // Rect &target = Elements[elementIndex].rect;
        // vector<LeafIterData> leaves;
        // leaves.reserve(64);
        // GetLeaves(
        //     quadNodeIndex,
        //     nodeRect,
        //     depth,
        //     target,
        //     &leaves);
        // for (int i = 0; i < leaves.size(); i++)
        // {
        //     int leafNodeIndex = leaves[i].nodeIndex;
        //     Rect leafNodeRect = leaves[i].rect;
        //     int leafDepth = leaves[i].depth;
        //     InsertLeafNode(leafNodeIndex, leafNodeRect, leafDepth, elementIndex);
        // }

        auto start_time = rclock::now();
        Rect &target = Elements[elementIndex].rect;
        FindLeaves(
            quadNodeIndex,
            nodeRect,
            depth,
            target,
            [this, elementIndex](int quadNodeIndex, Rect nodeRect, int depth)
            {
                this->InsertLeafNode(quadNodeIndex, nodeRect, depth, elementIndex);
            });
        auto end_time = rclock::now();
        METRICS.RecordQuadTreeInsertNode(end_time - start_time);
    }

    void GetLeaves(
        int quadNodeIndex,
        Rect &quadNodeRect,
        int depth,
        Rect target,
        vector<LeafIterData> *output)
    {
        vector<tuple<int, Rect, int>> stack;
        stack.push_back(make_tuple(quadNodeIndex, quadNodeRect, depth));

        QuadNode *current = nullptr;
        while (stack.size() > 0)
        {
            auto [currentIndex, rect, depth] = stack.back();
            current = &(Nodes[currentIndex]);
            stack.pop_back();

            if (current->IsLeaf())
            {
                output->push_back(LeafIterData{currentIndex, rect, depth});
            }
            else
            {
                if (target.L() <= rect.x)
                {
                    if (target.B() <= rect.y)
                    {
                        stack.push_back(make_tuple(current->BL(), rect.BL(), depth + 1));
                    }
                    if (target.T() > rect.y)
                    {
                        stack.push_back(make_tuple(current->TL(), rect.TL(), depth + 1));
                    }
                }
                if (target.R() > rect.x)
                {
                    if (target.B() <= rect.y)
                    {
                        stack.push_back(make_tuple(current->BR(), rect.BR(), depth + 1));
                    }
                    if (target.T() > rect.y)
                    {
                        stack.push_back(make_tuple(current->TR(), rect.TR(), depth + 1));
                    }
                }
            }
        }
    }

    void FindLeaves(int quadNodeIndex,
                    Rect &quadNodeRect,
                    int depth,
                    Rect &target,
                    LeafCallbackFn leafCallbackFn)
    {
        auto start_time = rclock::now();
        chrono::duration subtract_time = chrono::nanoseconds(0);

        vector<tuple<int, Rect, int>> stack;
        stack.push_back(make_tuple(quadNodeIndex, quadNodeRect, depth));

        QuadNode *current = nullptr;
        int num_nodes_processed = 0;
        int max_depth = max(0, depth);
        while (stack.size() > 0)
        {
            auto [currentIndex, rect, depth] = stack.back();
            current = &(Nodes[currentIndex]);
            stack.pop_back();
            num_nodes_processed += 1;
            max_depth = max(max_depth, depth);

            if (current->IsLeaf())
            {
                auto start_callback_time = rclock::now();
                leafCallbackFn(currentIndex, rect, depth);
                auto end_callback_time = rclock::now();
                subtract_time += (end_callback_time - start_callback_time);
            }
            else
            {
                if (target.L() <= rect.x)
                {
                    if (target.B() <= rect.y)
                    {
                        stack.push_back(make_tuple(current->BL(), rect.BL(), depth + 1));
                    }
                    if (target.T() > rect.y)
                    {
                        stack.push_back(make_tuple(current->TL(), rect.TL(), depth + 1));
                    }
                }
                if (target.R() > rect.x)
                {
                    if (target.B() <= rect.y)
                    {
                        stack.push_back(make_tuple(current->BR(), rect.BR(), depth + 1));
                    }
                    if (target.T() > rect.y)
                    {
                        stack.push_back(make_tuple(current->TR(), rect.TR(), depth + 1));
                    }
                }
            }
        }

        // printf("@@@@ num_nodes_processed = %d, %d\n", num_nodes_processed, max_depth);

        auto end_time = rclock::now();
        METRICS.RecordQuadFindLeaves((end_time - start_time) - subtract_time);
    }

    void InsertLeafNode(int quadNodeIndex, Rect nodeRect, int depth, int elementIndex)
    {
        auto start_time = rclock::now();
        QuadNode &node = Nodes[quadNodeIndex];
        int elementNodeIndex = ElementNodes.insert(QuadElementNode(elementIndex));

        // Update the node counter and pointers
        node.count += 1;
        if (node.count == 1)
        {
            node.children = elementNodeIndex;
        }
        else
        {
            QuadElementNode *newElementNode = &(ElementNodes[elementNodeIndex]);
            newElementNode->next = node.children;
            node.children = elementNodeIndex;
        }

        if (this->ShouldSplitNode(node) && depth < this->max_depth)
        {
            // Save the list of elementNodes
            vector<int> tempElementIndices;
            int tempIndex = node.children;
            while (tempIndex != -1)
            {
                QuadElementNode &en = ElementNodes[tempIndex];
                tempElementIndices.push_back(en.element);
                ElementNodes.erase(tempIndex);
                tempIndex = en.next;
            }

            // Create the new child QuadNodes
            int tr_index = this->Nodes.insert(QuadNode::Leaf()); // TR
            this->Nodes.insert(QuadNode::Leaf());                // TL
            this->Nodes.insert(QuadNode::Leaf());                // BL
            this->Nodes.insert(QuadNode::Leaf());                // BR
            this->Nodes.erase(quadNodeIndex);
            this->Nodes.insert(QuadNode::Branch(tr_index));

            // Insert the current node's Elements into the new quad nodes
            for (auto tempElementIndex : tempElementIndices)
            {
                InsertNode(quadNodeIndex, nodeRect, depth, tempElementIndex);
            }
        }

        auto end_time = rclock::now();
        METRICS.RecordQuadInsertLeafNode((end_time - start_time));
    }

    bool ShouldSplitNode(QuadNode &node)
    {
        return node.count >= split_threshold;
    }
};

class QuadTreeDraw
{
public:
    void *scene;
    SDL_Renderer *renderer;
    Mat3 &transform;

    QuadTreeDraw(void *scene, SDL_Renderer *renderer, Mat3 &transform) : scene(scene), renderer(renderer), transform(transform) {}
};

class Scene
{
public:
    // QuadTree quadTree;
    Quadtree quadTree;
    vector<Sprite> Sprites;
    Rect WorldBox;
    int cf;

    bool draw_quad_tree_rects = false;
    bool draw_sprite_rects = true;

    unordered_map<int, bool> sprites_processed;

    ~Scene()
    {
        qt_destroy(&quadTree);
    }

    void Build()
    {
        // quadTree = QuadTree(WorldBox, MAX_QUAD_TREE_DEPTH, QUAD_TREE_SPLIT_THRESHOLD);
        // for (Sprite &sprite : Sprites)
        // {
        //     int quadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        //     sprite.QuadId = quadId;
        // }

        qt_create(
            &quadTree,
            WorldBox.w,
            WorldBox.h,
            QUAD_TREE_SPLIT_THRESHOLD,
            MAX_QUAD_TREE_DEPTH);

        for (Sprite &sprite : Sprites)
        {
            sprite.QuadId = qt_insert(
                &quadTree,
                sprite.Id,
                (float)sprite.BoundingBox.L(),
                (float)sprite.BoundingBox.B(),
                (float)sprite.BoundingBox.R(),
                (float)sprite.BoundingBox.T());
        }
    }

    void QuadCollision()
    {
        auto start_time = rclock::now();
        sprites_processed.clear();
        qt_traverse(
            &quadTree,
            (void *)this,
            QuadHandleBranch,
            QuadHandleLeaf);

        // std::function<Sprite *(int id)> getDataFn = [&](int id) -> Sprite *
        // {
        //     return &Sprites[id];
        // };

        // unordered_map<int, bool> sprite_processed;
        // std::function<void(Sprite * sprite)> useDataFn = [this, getDataFn, &sprite_processed](Sprite *A)
        // {
        //     if (sprite_processed.count(A->Id) != 0)
        //     {
        //         return;
        //     }

        //     unordered_map<int, bool> child_processed;
        //     std::function<void(Sprite * sprite)> useDataFn = [this, &sprite_processed, &child_processed, A](Sprite *B)
        //     {
        //         if (A->Id == B->Id
        //             || child_processed.count(B->Id) != 0
        //             || sprite_processed.count(B->Id) != 0) {
        //             return;
        //         }

        //         if (A->Intersects(B))
        //         {
        //             A->IsColliding = true;
        //             B->IsColliding = true;
        //             A->Collides(B);
        //             child_processed[B->Id] = true;
        //         }
        //     };
        //     quadTree.QueryWithData(
        //         A->BoundingBox,
        //         getDataFn,
        //         useDataFn
        //     );
        //     sprite_processed[A->Id] = true;
        // };
        // quadTree.QueryWithData(
        //     WorldBox,
        //     getDataFn,
        //     useDataFn
        // );

        // auto end_time = rclock::now();
        // METRICS.RecordQuadCollision(end_time - start_time);

        // std::function<Sprite *(int id)> getDataFn = [&](int id) -> Sprite *
        // {
        //     return &Sprites[id];
        // };

        // for (int i = 0; i < Sprites.size(); i++)
        // {
        //     Sprite *A = &Sprites[i];

        //     unordered_map<int, bool> ps;
        //     std::function<void(Sprite * sprite)> useDataFn = [this, &ps, A](Sprite *B)
        //     {
        //         if (A->Id == B->Id || B->Id < A->Id)
        //         {
        //             return;
        //         }
        //         if (ps.find(B->Id) != ps.end())
        //         {
        //             return;
        //         }

        //         if (A->Intersects(B) && B->Intersects(A))
        //         {
        //             A->IsColliding = true;
        //             B->IsColliding = true;
        //             A->Collides(B);
        //             ps[B->Id] = true;
        //         }
        //     };
        //     quadTree.QueryWithData(
        //         A->BoundingBox,
        //         getDataFn,
        //         useDataFn
        //     );
        // }
    }

    void BruteCollision()
    {
        // Run collision logic
        Sprite *A = nullptr;
        Sprite *B = nullptr;
        for (int i = 0; i < Sprites.size(); i++)
        {
            A = &Sprites[i];
            for (int j = i + 1; j < Sprites.size(); j++)
            {
                // if ( i == j) {continue;}
                B = &Sprites[j];
                if (A->Intersects(B) && B->Intersects(A))
                {
                    A->IsColliding = true;
                    B->IsColliding = true;
                    Sprites[i].Collides(&Sprites[j]);
                }
            }
        }
    }

    void Update(chrono::milliseconds delta_ms)
    {
        for (Sprite &sprite : Sprites)
        {
            sprite.IsColliding = false;
        }

        if (USE_QUAD_TREE)
        {
            QuadCollision();
        }
        else
        {
            BruteCollision();
        }

        // Update the physics
        auto start_time = rclock::now();
        for (Sprite &sprite : Sprites)
        {
            sprite.Update(WorldBox, delta_ms);

            // // Update the quad tree with new position
            // quadTree.Remove(sprite.QuadId);
            // sprite.QuadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        }
        auto end_time = rclock::now();
        METRICS.RecordSpriteUpdatePhyscis(end_time - start_time);

        start_time = rclock::now();
        for (Sprite &sprite : Sprites)
        {
            // Update the quad tree with new position
            qt_remove(&quadTree, sprite.QuadId);
            sprite.QuadId = qt_insert(
                &quadTree,
                sprite.Id,
                (float)sprite.BoundingBox.L(),
                (float)sprite.BoundingBox.B(),
                (float)sprite.BoundingBox.R(),
                (float)sprite.BoundingBox.T());

            // quadTree.Remove(sprite.QuadId);
            // sprite.QuadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        }
        end_time = rclock::now();
        METRICS.RecordSpriteQuadTreeUpdate(end_time - start_time);
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {

        auto start_time = rclock::now();
        // quadTree.Draw(renderer, transform, delta_ms);
        // Draw the quad tree

        QuadTreeDraw draw((void *)this, renderer, transform);
        qt_traverse(&quadTree, &draw, QuadHandleDrawBranch, QuadHandleDrawLeaf);

        if (draw_sprite_rects)
        {
            for (Sprite &sprite : Sprites)
            {
                sprite.Draw(renderer, transform, delta_ms);
            }
        }

        auto end_time = rclock::now();
        METRICS.RecordSceneDraw(end_time - start_time);
    }

    void Clean()
    {
        qt_cleanup(&quadTree);
        // quadTree.Clean();
    }
};

void QuadHandleDrawBranch(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy)
{
    // QuadHandleDrawLeaf(qt, user_data, node, depth, mx, my, sx, sy);
}
void QuadHandleDrawLeaf(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy)
{
    QuadTreeDraw *draw = (QuadTreeDraw *)user_data;
    Scene *scene = (Scene *)draw->scene;
    SDL_Renderer *renderer = draw->renderer;
    Mat3 transform = draw->transform;

    // Rect currentRect = Rect(mx, my, sx, sy);
    // SDL_Rect rect = currentRect.ToSDL(transform);
    Vec2 p = transform * Vec2(mx - sx, my - sy);
    SDL_Rect rect;
    rect.x = (int)p.x;
    rect.y = (int)p.y;
    rect.w = 2 * sx;
    rect.h = 2 * sy;
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(renderer, &rect);

    if (scene->draw_quad_tree_rects == false)
    {
        return;
    }

    int nodeElementIndex = il_get(&(qt->nodes), node, node_idx_fc);
    int nodeElementCount = il_get(&(qt->nodes), node, node_idx_num);
    vector<int> elementIndices;
    while (nodeElementIndex != -1)
    {
        int nodeElementNext = il_get(&(qt->enodes), nodeElementIndex, enode_idx_next);
        int nodeElementElementIndex = il_get(&(qt->enodes), nodeElementIndex, enode_idx_elt);
        elementIndices.push_back(nodeElementElementIndex);
        nodeElementIndex = nodeElementNext;
    }

    unordered_map<int, bool> processed;
    for (int elementIndex : elementIndices)
    {
        int left = il_get(&(qt->elts), elementIndex, elt_idx_lft);
        int top = il_get(&(qt->elts), elementIndex, elt_idx_top);
        int right = il_get(&(qt->elts), elementIndex, elt_idx_rgt);
        int bottom = il_get(&(qt->elts), elementIndex, elt_idx_btm);

        Vec2 p = transform * Vec2(left, top);
        SDL_Rect rect;
        rect.x = (int)p.x;
        rect.y = (int)p.y;
        rect.w = right - left;
        rect.h = bottom - top;
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void QuadHandleBranch(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy)
{
    return;
}
void QuadHandleLeaf(Quadtree *qt, void *user_data, int node, int depth, int mx, int my, int sx, int sy)
{
    Scene *scene = (Scene *)user_data;

    int nodeElementIndex = il_get(&(qt->nodes), node, node_idx_fc);
    int nodeElementCount = il_get(&(qt->nodes), node, node_idx_num);

    vector<int> elementIndices;
    while (nodeElementIndex != -1)
    {
        int nodeElementNext = il_get(&(qt->enodes), nodeElementIndex, enode_idx_next);
        int nodeElementElementIndex = il_get(&(qt->enodes), nodeElementIndex, enode_idx_elt);
        elementIndices.push_back(nodeElementElementIndex);
        nodeElementIndex = nodeElementNext;
    }

    unordered_map<int, bool> processed;
    IntList temp_element_list;
    il_create(&temp_element_list, 0);

    // vector<int> sprite_ids;
    for (int elementIndex : elementIndices)
    {
        int left = il_get(&(qt->elts), elementIndex, elt_idx_lft);
        int top = il_get(&(qt->elts), elementIndex, elt_idx_top);
        int right = il_get(&(qt->elts), elementIndex, elt_idx_rgt);
        int bottom = il_get(&(qt->elts), elementIndex, elt_idx_btm);
        int sprite_id = il_get(&(qt->elts), elementIndex, elt_idx_id);

        Sprite *A = &scene->Sprites[sprite_id];
        if (scene->sprites_processed.count(elementIndex) != 0)
        {
            continue;
        }

        if (A->BoundingBox.L() != left)
        {
            printf("Error: node %d, left %d != %d\n", A->Id, A->BoundingBox.L(), left);
        }
        if (A->BoundingBox.B() != top)
        {
            printf("Error: node %d, top %d != %d\n", A->Id, A->BoundingBox.B(), top);
        }
        if (A->BoundingBox.R() != right)
        {
            printf("Error: node %d, right %d != %d\n", A->Id,  A->BoundingBox.R(), right);
        }
        if (A->BoundingBox.T() != bottom)
        {
            printf("Error: node %d, bottom %d != %d\n", A->Id, A->BoundingBox.T(), bottom);
        }
        
        qt_query(
            qt,
            &temp_element_list,
            (float)A->BoundingBox.L(),
            (float)A->BoundingBox.B(),
            (float)A->BoundingBox.R(),
            (float)A->BoundingBox.T(),
            elementIndex);

        for (int i = 0; i < temp_element_list.num; i++)
        {
            int other_element_id = il_get(&temp_element_list, i, 0);
            int other_sprite_id = il_get(&(qt->elts), other_element_id, elt_idx_id);
            Sprite *B = &scene->Sprites[other_sprite_id];

            if (A->already_collided.count(B->Id) != 0 || 
                B->already_collided.count(A->Id) != 0)
            {
                continue;
            }
            if (A->Intersects(B)) {
                A->IsColliding = true;
                B->IsColliding = true;
                A->Collides(B);
                A->already_collided[B->Id] = true;
                B->already_collided[A->Id] = true;
            }
        }

        scene->sprites_processed[elementIndex] = true;
        il_clear(&temp_element_list);
    }

    il_destroy(&temp_element_list);
}

class Game
{
public:
    int width;
    int height;
    Rect WorldBox;
    Scene scene;

    Game()
    {
        // Initialize random seed
        // srand(static_cast<unsigned int>(time(nullptr)));
        srand(1250);

        printf("World Width = %d, World Height = %d\n", WORLD_WIDTH, WORLD_HEIGHT);
        printf("Max Depth = %d\n", MAX_QUAD_TREE_DEPTH);
        printf("Max Rect Size = %d\n", MAX_RECT_SIZE);

        width = WORLD_WIDTH;
        height = WORLD_HEIGHT;
        // WorldBox = Rect(0, 0, width, height);
        WorldBox = Rect(width / 2, height / 2, width, height);

        // Create the scene
        scene.WorldBox = WorldBox;
        for (int i = 0; i < NUMBER_SPRITES; ++i)
        {
            // float posX = (float)(rand() % WorldBox.w) - WorldBox.W2();
            // float posY = (float)(rand() % WorldBox.h) - WorldBox.H2();
            // float posX = (float)(rand() % WorldBox.W2()) - WorldBox.W4();
            // float posY = (float)(rand() % WorldBox.H2()) - WorldBox.H4();
            float posX = (float)(rand() % WorldBox.w);
            float posY = (float)(rand() % WorldBox.h);
            float velX = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            float velY = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            int w = MIN_RECT_SIZE + (rand() % MAX_RECT_SIZE);
            // int h = MIN_RECT_SIZE + (rand() % MAX_RECT_SIZE);
            int h = w;

            Rect bounds = Rect((int)posX, (int)posY, w, h);
            scene.Sprites.push_back(
                Sprite(i,
                       Vec2(posX, posY),
                       Vec2(velX, velY),
                       bounds));
        }
        scene.Build();
    }

    ~Game() = default;

    void UpdatePhysics(chrono::milliseconds delta_ms)
    {
        scene.Update(delta_ms);
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        scene.Draw(renderer, transform, delta_ms);
        scene.Clean();
    }
};

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Quad Trees",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        VIEWPORT_WIDTH,
        VIEWPORT_HEIGHT,
        0);
    if (!window)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    Game game;
    bool quit = false;
    bool paused = false;
    auto start = rclock::now();

    Mat3 transform;
    transform = Mat3::Scale(1, 1) * Mat3::Translate(0, 0);
    // transform = Mat3::Scale(1, -1) * Mat3::Translate(VIEWPORT_WIDTH / 2, VIEWPORT_HEIGHT / 2);

    auto game_start_time = rclock::now();

    int frame_count = 0;
    double total_time_sec = 0;
    while (!quit)
    {
        auto frame_start_time = rclock::now();

        SDL_Event e;
        // Handle input
        while (SDL_PollEvent(&e) != 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                switch (e.key.keysym.sym)
                {
                case SDLK_w:
                    // transform = Mat3::Translate(0, -10) * transform;
                    transform = Mat3::Translate(0, 10) * transform;
                    break;
                case SDLK_s:
                    // transform = Mat3::Translate(0, 10) * transform;
                    transform = Mat3::Translate(0, -10) * transform;
                    break;
                case SDLK_a:
                    transform = Mat3::Translate(10, 0) * transform;
                    break;
                case SDLK_d:
                    transform = Mat3::Translate(-10, 0) * transform;
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.sym)
                {
                case SDLK_SPACE:
                    paused = !paused;
                    break;
                case SDLK_f:
                    game.scene.draw_quad_tree_rects = !game.scene.draw_quad_tree_rects;
                    break;
                case SDLK_r:
                    game.scene.draw_sprite_rects = !game.scene.draw_sprite_rects;
                    break;
                }
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderClear(renderer);
        auto now = rclock::now();
        auto delta_ms = chrono::duration_cast<chrono::milliseconds>(now - start);

        // Update Physics
        if (delta_ms >= chrono::milliseconds(16))
        {
            // printf("@@@@ delta_ms: %lld\n", delta_ms.count());
            start = now;
            if (!paused)
            {
                game.UpdatePhysics(delta_ms);
                // game.UpdatePhysics(chrono::milliseconds(16));
            }
        }

        // Draw Stuff
        game.Draw(renderer, transform, delta_ms);

        // Draw origin
        Vec2 origin = transform * Vec2(0, 0);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderDrawPoint(renderer, (int)origin.x, (int)origin.y);

        SDL_RenderPresent(renderer);

        auto frame_end_time = rclock::now();
        auto frame_delta_ms = chrono::duration_cast<chrono::milliseconds>(frame_end_time - frame_start_time);
        total_time_sec += (frame_delta_ms.count() / 1000.0);
        frame_count++;
        if (frame_count % 60 == 0)
        {
            printf("Frame Rate: %.2f fps\n", frame_count / total_time_sec);
        }
        current_frame += 1;
    }

    auto game_end_time = rclock::now();
    METRICS.RecordTotalTime(game_end_time - game_start_time);
    METRICS.Print();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}