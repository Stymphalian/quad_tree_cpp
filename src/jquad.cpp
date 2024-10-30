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
        const int nd_index = output.GetIndex(i);

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
}

void QuadTree::Query(Rect query, unordered_map<int, bool>& seen, vector<int> *output)
{
    const int left = query.L();
    const int top = query.T();
    const int right = query.R();
    const int bottom = query.B();

    LeavesListIntList leaves;
    FindLeavesList(
        ROOT_QUAD_NODE_INDEX,
        Bounds.mid_x, Bounds.mid_y, Bounds.half_w, Bounds.half_h,
        0,
        left, top, right, bottom,
        leaves);

    for (int i = 0; i < leaves.il_size(); i++)
    {
        const int nd_index = leaves.GetIndex(i);

        int elementNodeIndex = Nodes.GetChildren(nd_index);
        while (elementNodeIndex != -1)
        {
            int elementIndex = ElementNodes.GetElementId(elementNodeIndex);
            elementNodeIndex = ElementNodes.GetNext(elementNodeIndex);
            if (seen.find(elementIndex) != seen.end())
            {
                continue;
            }

            Rect elementRect = Elements.GetRect(elementIndex);
            if (query.Intersects(elementRect))
            {
                output->push_back(elementIndex);
            }
            seen[elementIndex] = true;
        }
    }
}

void QuadTree::Traverse(void *userData, QueryCallback branchCallback, QueryCallback leafCallback)
{
    vector<tuple<int, Rect, int>> stack;
    stack.push_back(make_tuple(ROOT_QUAD_NODE_INDEX, Bounds.ToRect(), 0));
    while(stack.size() > 0) {
        auto [nd_index, rect, depth] = stack.back();
        stack.pop_back();

        if (Nodes.IsLeaf(nd_index)) {
            if (leafCallback != nullptr) {
                leafCallback(userData, this, nd_index, rect, depth);
            }
        } else {
            if (branchCallback != nullptr) {
                branchCallback(userData, this, nd_index, rect, depth);
            }
            const int child = Nodes.GetChildren(nd_index);
            stack.push_back(make_tuple(child + 0, rect.TL(), depth + 1));
            stack.push_back(make_tuple(child + 1, rect.TR(), depth + 1));
            stack.push_back(make_tuple(child + 2, rect.BL(), depth + 1));
            stack.push_back(make_tuple(child + 3, rect.BR(), depth + 1));
        }
    }
}

void QuadTree::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms, bool render_rects)
{
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

            if (!render_rects)
            {
                continue;
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

    vector<int> to_process;
    to_process.reserve(64);
    to_process.push_back(ROOT_QUAD_NODE_INDEX);
    while (to_process.size() > 0)
    {
        int currentIndex = to_process.back();
        to_process.pop_back();

        // Loop through all the children.
        // count the number of leaves and insert any branch nodes to process
        int empty_child_leaf_count = 0;
        int child = Nodes.GetChildren(currentIndex);
        for (int i = 0; i < 4; i++)
        {
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
        const int nd_mx = output.GetMx(i);
        const int nd_my = output.GetMy(i);
        const int nd_sx = output.GetSx(i);
        const int nd_sy = output.GetSy(i);
        const int nd_index = output.GetIndex(i);
        const int nd_depth = output.GetDepth(i);
        InsertLeafNode(
            nd_index,
            nd_mx, nd_my, nd_sx, nd_sy,
            nd_depth,
            elementIndex);
    }
}

void QuadTree::FindLeavesList(
    int quadNodeIndex,
    int mid_x, int mid_y, int half_w, int half_h,
    int depth,
    int left, int top, int right, int bottom,
    LeavesListIntList &output)
{
    LeavesListIntList stack;
    stack.Add(quadNodeIndex, depth, mid_x, mid_y, half_w, half_h);

    while (stack.il_size() > 0)
    {
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
            output.Add(currentIndex, depth, nd_mx, nd_my, nd_sx, nd_sy);
        }
        else
        {
            const int child = Nodes.GetChildren(currentIndex);
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
                    stack.Add(child + 0, depth + 1, l, t, w4, h4);
                }
                if (right > nd_mx) // TR
                {
                    stack.Add(child + 1, depth + 1, r, t, w4, h4);
                }
            }
            if (bottom < nd_my)
            {
                if (left <= nd_mx) // BL
                {
                    stack.Add(child + 2, depth + 1, l, b, w4, h4);
                }
                if (right > nd_mx) // BR
                {
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

    if ((currentCount + 1) >= split_threshold && depth < this->max_depth)
    {
        // Save the list of elementNodes
        vector<int> tempElementIndices;
        tempElementIndices.reserve(8);

        int tempIndex = elementNodeIndex;
        while (tempIndex != -1)
        {
            int next = ElementNodes.GetNext(tempIndex);
            int saveElementId = ElementNodes.GetElementId(tempIndex);
            tempElementIndices.push_back(saveElementId);
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
        for (auto tempElementIndex : tempElementIndices)
        {
            InsertNode(quadNodeIndex, mid_x, mid_y, half_w, half_h, depth, tempElementIndex);
        }
    }
}