
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

#include "bitmap_image.hpp"
#include "free_list.hpp"
#include "jmath.h"

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

typedef int Identifier;
Identifier getNextId() {
    static Identifier Id = 0;
    return ++Id;
}

class Identifiable {
public:
    Identifier Id;
    Identifiable() : Id(getNextId()) {}
    ~Identifiable() = default;
};

class Point: public Identifiable{
public:
    int x;
    int y;
    Point() : Identifiable(), x(0), y(0) {}
    Point(Identifier id, int x, int y) : Identifiable(), x(x), y(y) {
        this->Id = id;
    }
    ~Point() = default;
};

class Rect {
public:
    // x,y is center of rect, w,h is full width and height
    int x;
    int y;
    int w;
    int h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x, int y, int w, int h): x(x), y(y), w(w), h(h) {}
    ~Rect() = default;

    inline const int L() { return x - w >> 1; }
    inline const int R() { return x + w >> 1; }
    inline const int T() { return y + h >> 1; }
    inline const int B() { return y - h >> 1; }
    inline const int Ox() { return x; }
    inline const int Oy() { return y; }
    inline const int W2() { return w >> 1; }
    inline const int H2() { return h >> 1; }
    inline Rect TR() {return Rect(x, y, W2(), H2());}    
    inline Rect TL() {return Rect(x - W2(), y, W2(), H2());}
    inline Rect BL() {return Rect(x - W2(), y - H2(), W2(), H2());}
    inline Rect BR() {return Rect(x, y - H2(), W2(), H2());}

    bool Intersects(Rect& r) {
        return (
            r.L() < R() && 
            r.R() > L() &&
            r.T() > B() && 
            r.B() < T()
        );
    }
    bool Contains(const Point& p)  {
        return (
            p.x >= x && p.x <= w && p.y >= y && p.y <= h
        );
    }
};


struct QuadNode {
    // If a branch node then the points to the first child QuadNode, 
    // else it points to the first Element of the leaf
    int32_t children;
    // Number of children in the leaf. -1 if a branch node.
    int32_t count;

    bool IsBranch() {
        return count == -1;
    }
    bool IsLeaf() {
        return count >= 0;
    }

    QuadNode() : children(-1), count(-1) {}
    inline int TR() {return children + 0;}
    inline int TL() {return children + 1;}
    inline int BL() {return children + 2;}
    inline int BR() {return children + 3;}
};

// Represents an element in the quadtree.
struct QuadElement {
    // Stores the ID for the element (can be used to
    // refer to external data).
    int id;

    // Stores the rectangle for the element.
    Rect rect;
    // int x1, y1, x2, y2;

    QuadElement(int id, Rect rect) : id(id), rect(rect) {}
};

// Represents an element node in the quadtree.
struct QuadElementNode {
    // Points to the next element in the leaf node. A value of -1 
    // indicates the end of the list.
    int next;

    // Stores the element index.
    int element;

    QuadElementNode(int elementIndex) : next(-1), element(elementIndex) {}
};


template <typename T>
class QuadTree : public Identifiable{
public:
    // Origin is center, x+ is right, y+ is up
    FreeList<QuadElement> Elements;
    FreeList<QuadElementNode> ElementNodes;
    FreeList<QuadNode> Nodes;
    Rect Bounds;
    // int free_node = -1;
    int split_threshold = 3;
    int max_depth;

    using Callback = std::function<void(QuadTree* tree, Identifier id)>;
    using LeafCallbackFn = std::function<void(QuadTree* tree, int quadNodeIndex, Rect& nodeRect, int depth)>;

    QuadTree(Rect bounds, int max_depth): Identifiable(), max_depth(max_depth) {
        this->Bounds = bounds;
        this->max_depth = 5;
    };
    ~QuadTree()  {
    }

    int Insert(Identifiable id, Rect& rect) {
        int elementIndex = Elements.insert(QuadElement(id, rect));
        int elementNodeIndex = ElementNodes.insert(QuadElementNode(elementIndex));
        InsertNode(0, Bounds, 0, elementNodeIndex);
        return elementIndex;
    }

    void Remove(int elementIndex) {
        int rootNodeIndex = 0;
        int depth = 0;
        FindLeaves(
            rootNodeIndex,
            Bounds,
            depth,
            Elements[elementIndex].rect,
            [this, elementIndex] (QuadTree* tree, int quadNodeIndex, Rect nodeRect, int depth) {
                QuadNode* node = Nodes[quadNodeIndex];
                node->count -= 1;

                int beforeIndex = -1;
                int currentIndex = node->children;
                QuadElementNode* current = nullptr;
                while (currentIndex != -1) {
                    current = ElementNodes[currentIndex];
                    if (current->element == elementIndex) {
                        break;
                    }
                    beforeIndex = currentIndex;
                    currentIndex = current->next;
                }

                if (beforeIndex == -1) {
                    node->children = current->next;
                } else {
                    QuadElementNode* before = ElementNodes[beforeIndex];
                    before->next = current->next;
                }
                ElementNodes.erase(currentIndex);
            }
        );
        Elements.erase(elementIndex);
    }

    void Query(Rect& targetRect, LeafCallbackFn callback) {
        int rootNodeIndex = 0;
        int depth = 0;
        FindLeaves(
            rootNodeIndex,
            Bounds,
            depth,
            targetRect,
            callback
        );
    }

    void InsertNode(int quadNodeIndex, Rect& nodeRect, int depth, int elementNodeIndex) {
        // Find all the leaves
        QuadElement& element = Elements[ElementNodes[elementNodeIndex].element];
        Rect& target = element.rect;
        FindLeaves(
            quadNodeIndex,
            nodeRect,
            depth,
            target,
            [elementNodeIndex](QuadTree* tree, int quadNodeIndex, Rect nodeRect, int depth) {
                tree->SplitLeafNode(quadNodeIndex, nodeRect, depth, elementNodeIndex);
            }
        );
    }

    void FindLeaves(int quadNodeIndex, Rect& quadNodeRect, int depth, Rect& target, LeafCallbackFn leafCallbackFn) {
        vector<tuple<int, Rect, int>> stack;
        stack.push_back(make_tuple(quadNodeIndex, quadNodeRect, depth));

        QuadNode* current = nullptr;
        int currentIndex;
        Rect rect;
        while(stack.size() > 0) {
            auto t = stack.back();
            currentIndex = get<0>(t);
            rect = get<1>(t);
            depth = get<2>(t);
            current = Nodes[currentIndex];
            stack.pop_back();

            if (current->IsLeaf()) {
                leafCallbackFn(this, currentIndex, rect, depth);
            } else {
                if (target.L() <= rect.x) {
                    if (target.B() <= rect.y) {
                        stack.push_back(make_tuple(current->BL(), rect.BL(), depth + 1));
                    }
                    if (target.T() > rect.y) {
                        stack.push_back(make_tuple(current->TL(), rect.TL(), depth + 1));
                    }
                }
                if (target.R() > rect.x) {
                    if (target.B() <= rect.y) {
                        stack.push_back(make_tuple(current->BR(), rect.BR(), depth + 1));
                    }
                    if (target.T() > rect.y) {
                        stack.push_back(make_tuple(current->TR(), rect.TR(), depth + 1));
                    }
                }
            }
        }
    }

    void SplitLeafNode(int quadNodeIndex, Rect nodeRect, int depth, int elementNodeIndex) {
        QuadNode* node = Nodes[quadNodeIndex];

        // Update the node counter and pointers
        node->count += 1;
        if (node->count == 1){
            node->children = elementNodeIndex;
        } else {
            QuadElementNode* qen = ElementNodes[node->children];
            qen->next = node->children;
            node->children = elementNodeIndex;
        }

        if (this->ShouldSplit(node) && depth < this->max_depth) {
            // Save the list of elementNodes
            vector<int> tempElementNodeIndices;
            int tempIndex = node->children;
            while (tempIndex != -1) {
                tempElementNodeIndices.push_back(tempIndex);
            }

            // Create the new child QuadNodes
            // int tr_index = (int) this->Nodes.size() + 1;
            int tr_index = this->Nodes.insert(QuadNode()); // TR
            this->Nodes.insert(QuadNode()); // TL
            this->Nodes.insert(QuadNode()); // BL
            this->Nodes.insert(QuadNode()); // BR
            node->count = 0;
            node->children = tr_index;

            // Insert the current node's Elements into the new quad nodes
            for(auto qenIndex : tempElementNodeIndices) {
                InsertNode(quadNodeIndex, nodeRect, depth, qenIndex);
            }
        }
    }


    bool ShouldSplitNode(QuadNode* node) {
        return node->count >= split_threshold;
    }
};


// void PrintBox(QuadTree* tree, vector<Point>& points, Identifier id) {
//     Point& point = points[id];
//     printf("(%d,%d):%d: (%d, %d)\n", tree->Id, tree->Depth, point.Id, point.x, point.y);
// }

class Sprite {
public:
    Vector2 Position;
    Vector2 Velocity;
    Rect BoundingBox;

    Sprite(Vector2 position, Vector2 velocity, Rect BB) {
        this->Position = position;
        this->Velocity = velocity;
        this->BoundingBox = BB;
    };
    ~Sprite() = default;

    
    void Update(Rect& bounds, chrono::milliseconds delta) {
        this->Position = this->Position + this->Velocity * delta.count();
        if (this->Position.x < bounds.L()) {
            this->Position.x = bounds.L();
            this->Velocity.x = -this->Velocity.x;
        } else if (this->Position.x > bounds.R()) {
            this->Position.x = bounds.R();
            this->Velocity.x = -this->Velocity.x;
        }
        if (this->Position.y < bounds.B()) {
            this->Position.y = bounds.B();
            this->Velocity.y = -this->Velocity.y;
        } else if (this->Position.y > bounds.T()) {
            this->Position.y = bounds.T();
            this->Velocity.y = -this->Velocity.y;
        }
    }
};


// typedef std::vector<QuadNode> QuadNodeList;

// static QuadNodeList find_leaves(const Quadtree& tree, const QuadNode& root, const int rect[4])
// {
//     QuadNodeList leaves, to_process;
//     to_process.push_back(root);
//     while (to_process.size() > 0)
//     {
//         const QuadNode nd = to_process.pop_back();

//         // If this node is a leaf, insert it to the list.
//         if (tree.nodes[nd.index].count != -1)
//             leaves.push_back(nd);
//         else
//         {
//             // Otherwise push the children that intersect the rectangle.
//             const int mx = nd.crect[0], my = nd.crect[1];
//             const int hx = nd.crect[2] >> 1, hy = nd.crect[3] >> 1;
//             const int fc = tree.nodes[nd.index].first_child;
//             const int l = mx-hx, t = my-hx, r = mx+hx, b = my+hy;

//             if (rect[1] <= my)
//             {
//                 if (rect[0] <= mx)
//                     to_process.push_back(child_data(l,t, hx, hy, fc+0, nd.depth+1));
//                 if (rect[2] > mx)
//                     to_process.push_back(child_data(r,t, hx, hy, fc+1, nd.depth+1));
//             }
//             if (rect[3] > my)
//             {
//                 if (rect[0] <= mx)
//                     to_process.push_back(child_data(l,b, hx, hy, fc+2, nd.depth+1));
//                 if (rect[2] > mx)
//                     to_process.push_back(child_data(r,b, hx, hy, fc+3, nd.depth+1));
//             }
//         }
//     }
//     return leaves;
// }


class Scene {
public:
    vector<Sprite> Sprites;
    Rect WorldBox;

    void Update(chrono::milliseconds delta_ms) {
        for (Sprite& sprite : Sprites) {
            sprite.Update(WorldBox, delta_ms);
        }
    }

    void Draw(image_drawer* drawer, float progress) {       
        drawer->pen_color(255, 0, 0);
        int width = WorldBox.W2();
        int height = WorldBox.H2();
        for (Sprite& sprite : Sprites) {
            int ox = (int) sprite.Position.x + width;
            int oy = (int) sprite.Position.y + height;
            drawer->plot_pixel(ox, oy);

            drawer->rectangle(
                ox - sprite.BoundingBox.W2(),
                oy - sprite.BoundingBox.H2(),
                ox + sprite.BoundingBox.W2(),
                oy + sprite.BoundingBox.H2()
            );
        }
    }
};

int main() {
    // Initialize random seed
    // srand(static_cast<unsigned int>(time(nullptr)));
    srand(100);

    vector<Point> points;
    int width = 1000;
    int height = 1000;
    Rect WorldBox = Rect(0, 0, width, height);
    // QuadTree tree(0, WorldBox);
    // for (int i = 0; i < 100; ++i) {
    //     int left = (rand() % 100) - tree.Bounds.HalfWidth();
    //     int right = (rand() % 100) - tree.Bounds.HalfHeight();
    //     points.push_back(Point(i, left,right));
    //     tree.Insert(points.back());
    // }
    // tree.Iterate([&points] (QuadTree* tree, Identifier id) {
    //     PrintBox(tree, points, id);
    // });
    // tree.Print();

    // Create the scene
    Scene scene;
    scene.WorldBox = WorldBox;
    for (int i = 0; i < 10; ++i) {
        float posX = (rand() % WorldBox.w) - WorldBox.W2();
        float posY = (rand() % WorldBox.h) - WorldBox.H2();
        float velX = ((rand() % 1000) / 1000.0)*3 - 1;
        float velY = ((rand() % 1000) / 1000.0)*3 - 1;
        int w = 10 + rand() % 40;
        int h = 10 + rand() % 40;
        Rect bounds = Rect(posX, posY, w, h);
        scene.Sprites.push_back(
            Sprite(Vector2(posX, posY), Vector2(velX, velY), bounds)
        );
    }

    bitmap_image image(WorldBox.w, WorldBox.h);
    image_drawer drawer(image);
    auto start = rclock::now();

    int num_frames_to_render = 100;
    for(int i = 0; i < num_frames_to_render; ++i) {
        auto now = rclock::now();
        auto delta_ms = chrono::duration_cast<chrono::milliseconds>(now - start);
        start = now;

        scene.Update(delta_ms);
        scene.Draw(&drawer, i/(float)num_frames_to_render);        
        image.save_image("image-" + to_string(i) + ".bmp");
        image.clear();
    }
    
    return 0;
}