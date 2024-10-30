#include <SDL_render.h>
#include "jquad.h"

QuadTree::QuadTree(Rect bounds, int max_depth, int split_threshold) : max_depth(max_depth), split_threshold(split_threshold)
{
    Nodes.AddLeaf();
    Bounds = QuadRect(bounds.x, bounds.y, bounds.w >> 1, bounds.h >> 1);
};
QuadTree::~QuadTree()
{
}

int QuadTree::Insert(int id, Rect &rect)
{
    int elementIndex = Elements.Add(
        id,
        rect.x - (rect.w >> 1),
        rect.y + (rect.h >> 1),
        rect.x + (rect.w >> 1),
        rect.y - (rect.h >> 1));
    InsertNode(ROOT_QUAD_NODE_INDEX,
               Bounds.mid_x, Bounds.mid_y, Bounds.half_w, Bounds.half_h,
               0,
               elementIndex);

    return elementIndex;
}

void QuadTree::Remove(int removeElementIndex)
{
    LeavesListIntList output;
    const int left = Elements.GetLeft(removeElementIndex);
    const int right = Elements.GetRight(removeElementIndex);
    const int top = Elements.GetTop(removeElementIndex);
    const int bottom = Elements.GetBottom(removeElementIndex);

    FindLeavesList(
        ROOT_QUAD_NODE_INDEX,
        Bounds.mid_x, Bounds.mid_y, Bounds.half_w, Bounds.half_h,
        0,
        left, top, right, bottom,
        output);

    for (int i = 0; i < output.il_size(); i++)
    {
        int nd_index = output.il_get(i, 4);

        int beforeIndex = -1;
        int currentElementNodeIndex = Nodes.GetChildren(nd_index);
        while (currentElementNodeIndex != -1)
        {
            int elementIndex = ElementNodes.GetElementId(currentElementNodeIndex);
            if (removeElementIndex == elementIndex)
            {
                break;
            }
            beforeIndex = currentElementNodeIndex;
            currentElementNodeIndex = ElementNodes.GetNext(currentElementNodeIndex);
        }

        Nodes.SetCount(nd_index, Nodes.GetCount(nd_index) - 1);
        const int next = ElementNodes.GetNext(currentElementNodeIndex);
        if (beforeIndex == -1)
        {
            Nodes.SetChildren(nd_index, next);
        }
        else
        {
            ElementNodes.SetNext(beforeIndex, next);
        }
        ElementNodes.il_erase(currentElementNodeIndex);
    }
    Elements.il_erase(removeElementIndex);

    // FindLeaves(
    //     ROOT_QUAD_NODE_INDEX,
    //     // Bounds,
    //     Bounds.mid_x, Bounds.mid_y, Bounds.half_w, Bounds.half_h,
    //     depth,
    //     removeElementIndex,
    //     [this, removeElementIndex](int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth)
    //     {
    //         int beforeIndex = -1;
    //         int currentElementNodeIndex = Nodes.GetChildren(quadNodeIndex);
    //         while (currentElementNodeIndex != -1)
    //         {
    //             int elementIndex = ElementNodes.GetElementId(currentElementNodeIndex);
    //             int next = ElementNodes.GetNext(currentElementNodeIndex);

    //             if (removeElementIndex == elementIndex)
    //             {
    //                 break;
    //             }
    //             beforeIndex = currentElementNodeIndex;
    //             currentElementNodeIndex = next;
    //         }

    //         Nodes.SetCount(quadNodeIndex, Nodes.GetCount(quadNodeIndex) - 1);
    //         if (beforeIndex == -1)
    //         {
    //             Nodes.SetChildren(quadNodeIndex, ElementNodes.GetNext(currentElementNodeIndex));
    //         }
    //         else
    //         {
    //             ElementNodes.SetNext(beforeIndex, ElementNodes.GetNext(currentElementNodeIndex));
    //         }
    //         ElementNodes.il_erase(currentElementNodeIndex);
    //     });

    // Elements.il_erase(removeElementIndex);
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
    vector<tuple<int, QuadRect, int>> nodes;
    nodes.push_back(make_tuple(ROOT_QUAD_NODE_INDEX, this->Bounds, 0));
    while (nodes.size() > 0)
    {
        auto [currentIndex, currentRect, depth] = nodes.back();
        nodes.pop_back();

        if (Nodes.IsLeaf(currentIndex))
        {
            if (depth >= 0)
            {
                int alpha = 12 + (64 - depth * (64 / max_depth));
                SDL_Rect rect = currentRect.ToSDL(transform);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                SDL_RenderDrawRect(renderer, &rect);
            }

            int elementNodeIndex = Nodes.GetChildren(currentIndex);
            while (elementNodeIndex != -1)
            {
                int elementIndex = ElementNodes.GetElementId(elementNodeIndex);
                const int left = Elements.GetLeft(elementIndex);
                const int right = Elements.GetRight(elementIndex);
                const int top = Elements.GetTop(elementIndex);
                const int bottom = Elements.GetBottom(elementIndex);

                Vec2 newPos = transform * Vec2(left, top);
                SDL_Rect rect;
                rect.x = (int)newPos.x;
                rect.y = (int)newPos.y;
                rect.w = abs(right - left);
                rect.h = abs(top - bottom);
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderDrawRect(renderer, &rect);

                elementNodeIndex = ElementNodes.GetNext(elementNodeIndex);
                ;
            }
        }
        else if (Nodes.IsBranch(currentIndex))
        {
            int child = Nodes.GetChildren(currentIndex);
            nodes.push_back(make_tuple(
                child + 0,
                currentRect.TL(),
                depth + 1));
            nodes.push_back(make_tuple(
                child + 1,
                currentRect.TR(),
                depth + 1));
            nodes.push_back(make_tuple(
                child + 2,
                currentRect.BL(),
                depth + 1));
            nodes.push_back(make_tuple(
                child + 3,
                currentRect.BR(),
                depth + 1));
        }
    }
}

void QuadTree::Clean()
{
    if (Nodes.IsLeaf(ROOT_QUAD_NODE_INDEX))
    {
        return;
    }
    // auto start_time = rclock::now();

    vector<int> to_process;
    to_process.reserve(64);
    to_process.push_back(ROOT_QUAD_NODE_INDEX);
    while (to_process.size() > 0)
    {
        int currentIndex = to_process.back();
        to_process.pop_back();

        // Loop through all the children.
        // count the number of leaves and insert any branch nodes to process
        // bool all_leaves = true;
        int empty_child_leaf_count = 0;
        int child = Nodes.GetChildren(currentIndex);
        for (int i = 0; i < 4; i++)
        {
            // QuadNode &childNode = Nodes[i];
            if (Nodes.IsBranch(child + i))
            {
                to_process.push_back(child + i);
            }
            else
            {
                empty_child_leaf_count += Nodes.IsEmpty(child + i) ? 1 : 0;
            }
        }

        // If all the child nodes are leaves we can delete this branch
        // node and make it a leaf node again.
        if (empty_child_leaf_count == 4)
        {
            Nodes.il_erase(child + 3);
            Nodes.il_erase(child + 2);
            Nodes.il_erase(child + 1);
            Nodes.il_erase(child + 0);
            Nodes.il_erase(currentIndex);
            Nodes.AddLeaf();
        }
    }
    // auto end_time = rclock::now();
    // METRICS.RecordQuadClean(end_time - start_time);
}

void QuadTree::InsertNode(int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex)
{
    LeavesListIntList output;
    const int left = Elements.GetLeft(elementIndex);
    const int right = Elements.GetRight(elementIndex);
    const int top = Elements.GetTop(elementIndex);
    const int bottom = Elements.GetBottom(elementIndex);
    FindLeavesList(
        quadNodeIndex,
        mid_x, mid_y, half_w, half_h,
        depth,
        left, top, right, bottom,
        output);
    for (int i = 0; i < output.il_size(); i++)
    {
        int nd_mx = output.il_get(i, 0);
        int nd_my = output.il_get(i, 1);
        int nd_sx = output.il_get(i, 2);
        int nd_sy = output.il_get(i, 3);
        int nd_index = output.il_get(i, 4);
        int nd_depth = output.il_get(i, 5);

        InsertLeafNode(
            nd_index,
            nd_mx, nd_my, nd_sx, nd_sy, nd_depth,
            elementIndex);
    }
}

void QuadTree::FindLeavesList(
    int quadNodeIndex,
    int mid_x, int mid_y, int half_w, int half_h,
    int depth,
    int left, int top, int right, int bottom,
    // int elementIndex,
    LeavesListIntList &output)
{
    // vector<tuple<int, QuadRect, int>> stack;
    // stack.push_back(make_tuple(quadNodeIndex, quadNodeRect, depth));
    // const int left = Elements.GetLeft(elementIndex);
    // const int top = Elements.GetTop(elementIndex);
    // const int right = Elements.GetRight(elementIndex);
    // const int bottom = Elements.GetBottom(elementIndex);

    LeavesListIntList stack;
    stack.Add(quadNodeIndex, depth, mid_x, mid_y, half_w, half_h);

    while (stack.il_size() > 0)
    {
        // auto [currentIndex, rect, depth] = stack.back();
        // stack.pop_back();
        const int stack_index = stack.il_size() - 1;
        const int nd_mx = stack.GetMx(stack_index);
        const int nd_my = stack.GetMy(stack_index);
        const int nd_sx = stack.GetSx(stack_index);
        const int nd_sy = stack.GetSy(stack_index);
        const int currentIndex = stack.GetIndex(stack_index);
        const int depth = stack.GetDepth(stack_index);
        stack.il_pop_back();

        if (Nodes.IsLeaf(currentIndex))
        {
            // leafCallbackFn(currentIndex, QuadRect(nd_mx, nd_my, nd_sx, nd_sy), depth);
            // leafCallbackFn(currentIndex, nd_mx, nd_my, nd_sx, nd_sy, depth);
            output.Add(currentIndex, depth, nd_mx, nd_my, nd_sx, nd_sy);
        }
        else
        {
            const int child = Nodes.GetChildren(currentIndex);
            // const int w4 = rect.half_w >> 1;
            // const int h4 = rect.half_h >> 1;
            // const int l = rect.mid_x - w4;
            // const int r = rect.mid_x + w4;
            // const int t = rect.mid_y + h4;
            // const int b = rect.mid_y - h4;
            const int w4 = nd_sx >> 1;
            const int h4 = nd_sy >> 1;
            const int l = nd_mx - w4;
            const int r = nd_mx + w4;
            const int t = nd_my + h4;
            const int b = nd_my - h4;

            if (top >= nd_my)
            {
                if (left <= nd_mx) // TL
                {
                    // stack.push_back(make_tuple(child + 0, QuadRect(l, t, w4, h4), depth + 1));
                    stack.Add(child + 0, depth + 1, l, t, w4, h4);
                }
                if (right > nd_mx) // TR
                {
                    // stack.push_back(make_tuple(child + 1, QuadRect(r, t, w4, h4), depth + 1));
                    stack.Add(child + 1, depth + 1, r, t, w4, h4);
                }
            }
            if (bottom < nd_my)
            {
                if (left <= nd_mx) // BL
                {
                    // stack.push_back(make_tuple(child + 2, QuadRect(l, b, w4, h4), depth + 1));
                    stack.Add(child + 2, depth + 1, l, b, w4, h4);
                }
                if (right > nd_mx) // BR
                {
                    // stack.push_back(make_tuple(child + 3, QuadRect(r, b, w4, h4), depth + 1));
                    stack.Add(child + 3, depth + 1, r, b, w4, h4);
                }
            }
        }
    }
}

void QuadTree::InsertLeafNode(int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex)
{
    // Update the QuadNodes children to the new elementNode
    // Increment the counters
    const int nodeChild = Nodes.GetChildren(quadNodeIndex);
    const int currentCount = Nodes.GetCount(quadNodeIndex);
    const int elementNodeIndex = ElementNodes.Add(elementIndex);
    ElementNodes.SetNext(elementNodeIndex, nodeChild);
    Nodes.SetChildren(quadNodeIndex, elementNodeIndex);
    Nodes.SetCount(quadNodeIndex, currentCount + 1);

    if ((currentCount +1) >= split_threshold && depth < this->max_depth)
    {
        // Save the list of elementNodes
        // vector<int> tempElementIndices;
        // TODO: Need to make this variable...
        int tempElementIndices[8];
        int ti = 0;

        // int tempIndex = Nodes.GetChildren(quadNodeIndex);
        int tempIndex = elementNodeIndex;
        while (tempIndex != -1)
        {
            // QuadElementNode &en = ElementNodes[tempIndex];
            int next = ElementNodes.GetNext(tempIndex);
            int saveElementId = ElementNodes.GetElementId(tempIndex);
            // tempElementIndices.push_back(saveElementId);
            tempElementIndices[ti++] = saveElementId;
            ElementNodes.il_erase(tempIndex);
            tempIndex = next;
        }

        // Create the new child QuadNodes
        int tl_index = Nodes.AddLeaf(); // TL
        Nodes.AddLeaf();                // TR
        Nodes.AddLeaf();                // BL
        Nodes.AddLeaf();                // BR
        Nodes.MakeBranch(quadNodeIndex, tl_index);

        // Insert the current node's Elements into the new quad nodes
        // for (auto tempElementIndex : tempElementIndices)
        for (int j = 0; j < ti; j++)
        {
            // InsertNode(quadNodeIndex, mid_x, mid_y, half_w, half_h, depth, tempElementIndex);
            InsertNode(quadNodeIndex, mid_x, mid_y, half_w, half_h, depth, tempElementIndices[j]);
        }
    }
}