
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

// 13 FPS
const int WORLD_WIDTH = 10000;
const int WORLD_HEIGHT = 10000;
const int NUMBER_SPRITES = 10000;
const int MAX_SPRITE_VELOCITY = 120;
const int MAX_RECT_SIZE = 40;
const int MAX_QUAD_TREE_DEPTH = 8;
const bool USE_QUAD_TREE = true;




// const int WORLD_WIDTH = 200;
// const int WORLD_HEIGHT = 200;
// const int NUMBER_SPRITES = 10;
// const int MAX_SPRITE_VELOCITY = 80;
// const int MAX_RECT_SIZE = 10;

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

typedef int Identifier;
Identifier getNextId()
{
    static Identifier Id = 0;
    return ++Id;
}

class Identifiable
{
public:
    Identifier Id;
    Identifiable() : Id(getNextId()) {}
    ~Identifiable() = default;
};

class Point : public Identifiable
{
public:
    int x;
    int y;
    Point() : Identifiable(), x(0), y(0) {}
    Point(Identifier id, int x, int y) : Identifiable(), x(x), y(y)
    {
        this->Id = id;
    }
    ~Point() = default;
};

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

    bool Contains(const Point &p)
    {
        return (
            p.x >= x && p.x <= w && p.y >= y && p.y <= h);
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
    int Id;
    int QuadId;
    bool IsColliding = false;

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
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        SDL_RenderDrawPoint(renderer, (int)newPos.x, (int)newPos.y);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool Intersects(Sprite *other)
    {
        // return (this->Position - other->Position).Length()  < (this->BoundingBox.w + other->BoundingBox.w)/2;
        return this->BoundingBox.Intersects(other->BoundingBox);
    }

    void CollidesOriginal(Sprite *other)
    {
        // Vec2 Pos = this->Position - this->ForceDrift;
        // Vec2 OtherPos = other->Position - other->ForceDrift;

        // Vec2 dir = (OtherPos - Pos).Normalize();  // A to B
        // Vec2 rdir = (Pos - OtherPos).Normalize(); // B to A
        Vec2 dir = (other->Position - this->Position).Normalize();  // A to B
        Vec2 rdir = (this->Position - other->Position).Normalize(); // B to A
        Vec2 dir_norm = dir.GetNormal();
        Vec2 rdir_norm = rdir.GetNormal();

        Vec2 comp_dir_a = dir * this->Velocity.dot(dir);
        Vec2 comp_dir_b = rdir * other->Velocity.dot(rdir);
        Vec2 comp_norm_a = dir_norm * this->Velocity.dot(dir_norm);
        Vec2 comp_norm_b = rdir_norm * other->Velocity.dot(rdir_norm);

        this->Velocity = comp_norm_a + comp_dir_b;
        other->Velocity = comp_norm_b + comp_dir_a;
        this->Position = this->Position - dir;
        other->Position = other->Position - rdir;

        // If the sprites are colliding we need to slowly move them apart
        // this->ForceDrift += dir;
        // other->ForceDrift += rdir;
    }

    void Collides(Sprite *other)
    {
        Vec2 dir = (other->Position - this->Position).Normalize();  // A to B
        Vec2 rdir = (this->Position - other->Position).Normalize(); // B to A
        Vec2 dir_norm = dir.GetNormal();
        Vec2 rdir_norm = rdir.GetNormal();

        Vec2 comp_dir_a = dir * this->Velocity.dot(dir);
        Vec2 comp_dir_b = rdir * other->Velocity.dot(rdir);
        Vec2 comp_norm_a = dir_norm * this->Velocity.dot(dir_norm);
        Vec2 comp_norm_b = rdir_norm * other->Velocity.dot(rdir_norm);

        this->Velocity = comp_norm_a + comp_dir_b;
        other->Velocity = comp_norm_b + comp_dir_a;
        this->Position = this->Position - dir;
        other->Position = other->Position - rdir;
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
    QuadElement(Identifiable id, Rect rect) : id(id.Id), rect(rect) {}
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
    Rect Bounds;
    // int free_node = -1;
    int split_threshold = 3;
    int max_depth;

    using LeafCallbackFn = std::function<void(int quadNodeIndex, Rect &nodeRect, int depth)>;
    using LeafCallbackWithTreeFn = std::function<void(QuadTree *tree, int quadNodeIndex, Rect &nodeRect, int depth)>;

    // template<typename T> struct FunctionType
    // {
    //     typedef std::function<T(int id)> GetDataFn;
    //     typedef std::function<void(T* data)> UseDataFn;
    // };

    QuadTree() : max_depth(3), Bounds(Rect(0, 0, 100, 100))
    {
        Nodes.insert(QuadNode::Leaf());
    }
    QuadTree(Rect bounds, int max_depth) : Bounds(bounds), max_depth(max_depth)
    {
        Nodes.insert(QuadNode::Leaf());
    };
    ~QuadTree()
    {
    }

    int Insert(int id, Rect &rect)
    {
        int elementIndex = Elements.insert(QuadElement(id, rect));
        InsertNode(0, Bounds, 0, elementIndex);
        return elementIndex;
    }

    void Remove(int elementIndex)
    {
        // int rootNodeIndex = 0;
        int depth = 0;
        FindLeaves(
            ROOT_QUAD_NODE_INDEX,
            Bounds,
            depth,
            Elements[elementIndex].rect,
            [this, elementIndex](int quadNodeIndex, Rect nodeRect, int depth)
            {
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
            });
        Elements.erase(elementIndex);
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

    void InsertNode(int quadNodeIndex, Rect &nodeRect, int depth, int elementIndex)
    {
        Rect &target = Elements[elementIndex].rect;
        FindLeaves(
            quadNodeIndex,
            nodeRect,
            depth,
            target,
            [this, elementIndex](int quadNodeIndex, Rect nodeRect, int depth)
            {
                this->SplitLeafNode(quadNodeIndex, nodeRect, depth, elementIndex);
            });
    }

    void Clean()
    {
        if (Nodes[0].IsLeaf())
        {
            return;
        }

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
    }

    void FindLeaves(int quadNodeIndex,
                    Rect &quadNodeRect,
                    int depth,
                    Rect &target,
                    LeafCallbackFn leafCallbackFn)
    {
        vector<tuple<int, Rect, int>> stack;
        stack.push_back(make_tuple(quadNodeIndex, quadNodeRect, depth));

        QuadNode *current = nullptr;
        // int currentIndex;
        // Rect rect;
        while (stack.size() > 0)
        {
            // auto t = stack.back();
            auto [currentIndex, rect, depth] = stack.back();
            // currentIndex = get<0>(t);
            // rect = get<1>(t);
            // depth = get<2>(t);
            current = &(Nodes[currentIndex]);
            stack.pop_back();

            if (current->IsLeaf())
            {
                leafCallbackFn(currentIndex, rect, depth);
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

    void SplitLeafNode(int quadNodeIndex, Rect nodeRect, int depth, int elementIndex)
    {
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
    }

    bool ShouldSplitNode(QuadNode &node)
    {
        return node.count >= split_threshold;
    }

    void Update(chrono::milliseconds delta_ms)
    {
        Clean();
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        // TODO: Draw the quadtree
        vector<tuple<QuadNode *, Rect>> nodes;
        nodes.push_back(make_tuple(&(this->Nodes[0]), this->Bounds));
        while (nodes.size() > 0)
        {
            auto [current, currentRect] = nodes.back();
            nodes.pop_back();

            if (current->IsLeaf())
            {
                SDL_Rect rect = currentRect.ToSDL(transform);
                SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                SDL_RenderDrawRect(renderer, &rect);
            }
            else if (current->IsBranch())
            {
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TL()]),
                    currentRect.TL()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TR()]),
                    currentRect.TR()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BL()]),
                    currentRect.BL()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BR()]),
                    currentRect.BR()));
            }
        }
    }

    void Draw2(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms, vector<Sprite> &sprites)
    {
        // TODO: Draw the quadtree
        vector<tuple<QuadNode *, Rect>> nodes;
        nodes.push_back(make_tuple(&(this->Nodes[0]), this->Bounds));
        while (nodes.size() > 0)
        {
            auto [current, currentRect] = nodes.back();
            nodes.pop_back();

            if (current->IsLeaf())
            {
                SDL_Rect rect = currentRect.ToSDL(transform);
                SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                SDL_RenderDrawRect(renderer, &rect);

                int elementNodeIndex = current->children;
                while (elementNodeIndex != -1)
                {
                    QuadElementNode &en = ElementNodes[elementNodeIndex];
                    QuadElement &element = Elements[en.element];
                    // Sprite &sprite = sprites[element.id];
                    SDL_Rect spriteRect = element.rect.ToSDL(transform);
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                    SDL_RenderDrawRect(renderer, &spriteRect);
                    elementNodeIndex = en.next;
                }
            }
            else if (current->IsBranch())
            {
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TL()]),
                    currentRect.TL()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->TR()]),
                    currentRect.TR()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BL()]),
                    currentRect.BL()));
                nodes.push_back(make_tuple(
                    &(this->Nodes[current->BR()]),
                    currentRect.BR()));
            }
        }
    }
};

class Scene
{
public:
    QuadTree quadTree;
    vector<Sprite> Sprites;
    Rect WorldBox;

    void Build()
    {
        quadTree = QuadTree(WorldBox, MAX_QUAD_TREE_DEPTH);
        for (Sprite &sprite : Sprites)
        {
            int quadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
            sprite.QuadId = quadId;
        }
    }

    void QuadCollision()
    {
        std::function<Sprite *(int id)> getDataFn = [&](int id) -> Sprite *
        {
            return &Sprites[id];
        };

        for (int i = 0; i < Sprites.size(); i++)
        {
            Sprite *A = &Sprites[i];

            unordered_map<int, bool> ps;
            std::function<void(Sprite * sprite)> useDataFn = [this, &ps, A](Sprite *B)
            {
                if (A->Id == B->Id || B->Id < A->Id)
                {
                    return;
                }
                if (ps.find(B->Id) != ps.end())
                {
                    return;
                }

                if (A->Intersects(B) && B->Intersects(A))
                {
                    A->IsColliding = true;
                    B->IsColliding = true;
                    A->Collides(B);
                    ps[B->Id] = true;
                }
            };
            quadTree.QueryWithData(
                // quadTree.Bounds,
                A->BoundingBox,
                getDataFn,
                useDataFn);
        }
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

                    // Update the quad tree with new position
                    quadTree.Remove(A->QuadId);
                    A->QuadId = quadTree.Insert(A->Id, A->BoundingBox);
                    quadTree.Remove(B->QuadId);
                    B->QuadId = quadTree.Insert(B->Id, B->BoundingBox);
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
        for (Sprite &sprite : Sprites)
        {
            sprite.Update(WorldBox, delta_ms);
            // sprite.IsColliding = false;

            // Update the quad tree with new position
            quadTree.Remove(sprite.QuadId);
            sprite.QuadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        }
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        quadTree.Draw(renderer, transform, delta_ms);
        for (Sprite &sprite : Sprites)
        {
            sprite.Draw(renderer, transform, delta_ms);
        }

        // quadTree.Draw2(renderer, transform, delta_ms, Sprites);
    }

    void Clean()
    {
        quadTree.Clean();
    }
};

class Game
{
public:
    vector<Point> points;
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
        WorldBox = Rect(0, 0, width, height);

        // Create the scene
        scene.WorldBox = WorldBox;
        for (int i = 0; i < NUMBER_SPRITES; ++i)
        {
            float posX = (float)(rand() % WorldBox.w) - WorldBox.W2();
            float posY = (float)(rand() % WorldBox.h) - WorldBox.H2();
            float velX = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            float velY = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            int w = 10 + (rand() % MAX_RECT_SIZE);
            int h = 10 + (rand() % MAX_RECT_SIZE);
            Rect bounds = Rect((int)posX, (int)posY, w, h);
            scene.Sprites.push_back(
                Sprite(i, Vec2(posX, posY), Vec2(velX, velY), bounds));
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

    int viewportWidth = 640;
    int viewportHeight = 480;
    SDL_Window *window = SDL_CreateWindow(
        "Quad Trees",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        viewportWidth,
        viewportHeight,
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
    transform = Mat3::Scale(1, -1) * Mat3::Translate(viewportWidth / 2, viewportHeight / 2);

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
                    transform = Mat3::Translate(0, -10) * transform;
                    break;
                case SDLK_s:
                    transform = Mat3::Translate(0, 10) * transform;
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
                }
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
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

        // Rect mrect(50, 0, 100, 100);
        // SDL_Rect rect = mrect.ToSDL(transform);
        // SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        // SDL_RenderDrawRect(renderer, &rect);

        // Rect bl = mrect.BR();
        // rect = bl.ToSDL(transform);
        // SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        // SDL_RenderDrawRect(renderer, &rect);

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
            // printf("Total Time: %f sec\n", total_time_sec);
            printf("Frame Rate: %.2f fps\n", frame_count / total_time_sec);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}