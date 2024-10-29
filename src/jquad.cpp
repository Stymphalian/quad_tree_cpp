#include <SDL_render.h>
#include "jquad.h"

QuadTree::QuadTree() : max_depth(3), Bounds(Rect(0, 0, 100, 100)), split_threshold(3)
{
    Nodes.insert(QuadNode::Leaf());
}
QuadTree::QuadTree(Rect bounds, int max_depth, int split_threshold) : Bounds(bounds), max_depth(max_depth), split_threshold(split_threshold)
{
    Nodes.insert(QuadNode::Leaf());
};
QuadTree::~QuadTree()
{
}

int QuadTree::Insert(int id, Rect &rect)
{
    auto start_time = rclock::now();
    int elementIndex = Elements.insert(QuadElement(id, rect));
    InsertNode(0, Bounds, 0, elementIndex);
    auto end_time = rclock::now();
    METRICS.RecordQuadTreeInsert(end_time - start_time);

    return elementIndex;
}

void QuadTree::Remove(int elementIndex)
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

// void QuadTree::Query(Rect &queryRect, LeafCallbackWithTreeFn callback)
// {
//     int depth = 0;
//     FindLeaves(
//         ROOT_QUAD_NODE_INDEX,
//         Bounds,
//         depth,
//         queryRect,
//         [this, callback](int quadNodeIndex, Rect nodeRect, int depth)
//         {
//             callback(this, quadNodeIndex, nodeRect, depth);
//         });
// }

// template <typename T>
// void QuadTree::QueryWithData(
//     Rect &queryRect,
//     std::function<T *(int id)> getDataFn,
//     std::function<void(T *data)> useDataFn)
// {
//     int depth = 0;
//     FindLeaves(
//         ROOT_QUAD_NODE_INDEX,
//         Bounds,
//         depth,
//         queryRect,
//         [this, getDataFn, useDataFn](int quadNodeIndex, Rect nodeRect, int depth)
//         {
//             QuadNode &node = Nodes[quadNodeIndex];

//             int elementNodeIndex = node.children;
//             while (elementNodeIndex != -1)
//             {
//                 QuadElementNode &en = ElementNodes[elementNodeIndex];
//                 QuadElement &element = Elements[en.element];
//                 T *data = getDataFn(element.id);
//                 useDataFn(data);
//                 elementNodeIndex = en.next;
//             }
//         });
// }

void QuadTree::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
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
                int alpha = 12 + (64 - depth * (64 / max_depth));
                SDL_Rect rect = currentRect.ToSDL(transform);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
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

void QuadTree::Clean()
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

void QuadTree::InsertNode(int quadNodeIndex, Rect &nodeRect, int depth, int elementIndex)
{
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

void QuadTree::FindLeaves(int quadNodeIndex,
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

    auto end_time = rclock::now();
    METRICS.RecordQuadFindLeaves((end_time - start_time) - subtract_time);
}

void QuadTree::InsertLeafNode(int quadNodeIndex, Rect nodeRect, int depth, int elementIndex)
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

bool QuadTree::ShouldSplitNode(QuadNode &node)
{
    return node.count >= split_threshold;
}