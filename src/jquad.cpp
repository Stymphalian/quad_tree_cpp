#include <SDL_render.h>
#include "jquad.h"

QuadTree::QuadTree(Rect bounds, int maxDepth, int splitThreshold)
    : _maxDepth(maxDepth),
      _Bounds(bounds.x, bounds.y, bounds.w >> 1, bounds.h >> 1),
      _splitThreshold(splitThreshold)
{
    _Nodes.AddLeaf();
};
QuadTree::~QuadTree() {}

int QuadTree::Insert(int id, Rect &rect)
{
    const int elementIndex = _Elements.Add(
        id,
        rect.x - (rect.w >> 1),
        rect.y + (rect.h >> 1),
        rect.x + (rect.w >> 1),
        rect.y - (rect.h >> 1));
    InsertNode(ROOT_QUAD_NODE_INDEX,
               _Bounds.midX, _Bounds.midY, _Bounds.halfW, _Bounds.halfH,
               0,
               elementIndex);

    return elementIndex;
}

void QuadTree::Remove(int removeElementIndex)
{
    LeavesListIntList output;
    const int left = _Elements.GetLeft(removeElementIndex);
    const int right = _Elements.GetRight(removeElementIndex);
    const int top = _Elements.GetTop(removeElementIndex);
    const int bottom = _Elements.GetBottom(removeElementIndex);

    FindLeavesList(
        ROOT_QUAD_NODE_INDEX,
        _Bounds.midX, _Bounds.midY, _Bounds.halfW, _Bounds.halfH,
        0,
        left, top, right, bottom,
        output);

    for (int i = 0; i < output.size(); i++)
    {
        const int nodeIndex = output.GetIndex(i);

        int beforeIndex = -1;
        int currentElementNodeIndex = _Nodes.GetChildren(nodeIndex);
        while (currentElementNodeIndex != -1)
        {
            int elementIndex = _ElementNodes.GetElementId(currentElementNodeIndex);
            if (removeElementIndex == elementIndex)
            {
                break;
            }
            beforeIndex = currentElementNodeIndex;
            currentElementNodeIndex = _ElementNodes.GetNext(currentElementNodeIndex);
        }

        _Nodes.SetCount(nodeIndex, _Nodes.GetCount(nodeIndex) - 1);
        const int next = _ElementNodes.GetNext(currentElementNodeIndex);
        if (beforeIndex == -1)
        {
            _Nodes.SetChildren(nodeIndex, next);
        }
        else
        {
            _ElementNodes.SetNext(beforeIndex, next);
        }
        _ElementNodes.erase(currentElementNodeIndex);
    }
    _Elements.erase(removeElementIndex);
}

void QuadTree::Query(Rect query,
                     unordered_map<int, bool> &seen,
                     vector<int> *output)
{
    const int left = query.L();
    const int top = query.T();
    const int right = query.R();
    const int bottom = query.B();

    LeavesListIntList leaves;
    FindLeavesList(
        ROOT_QUAD_NODE_INDEX,
        _Bounds.midX, _Bounds.midY, _Bounds.halfW, _Bounds.halfH,
        0,
        left, top, right, bottom,
        leaves);

    for (int i = 0; i < leaves.size(); i++)
    {
        const int nodeIndex = leaves.GetIndex(i);

        int elementNodeIndex = _Nodes.GetChildren(nodeIndex);
        while (elementNodeIndex != -1)
        {
            int elementIndex = _ElementNodes.GetElementId(elementNodeIndex);
            elementNodeIndex = _ElementNodes.GetNext(elementNodeIndex);
            if (seen.find(elementIndex) != seen.end())
            {
                continue;
            }

            const int l = _Elements.GetLeft(elementIndex);
            const int t = _Elements.GetTop(elementIndex);
            const int r = _Elements.GetRight(elementIndex);
            const int b = _Elements.GetBottom(elementIndex);
            if (Intersects(left, top, right, bottom,
                           l, t, r, b))
            {
                output->push_back(elementIndex);
            }
            seen[elementIndex] = true;
        }
    }
}

void QuadTree::Traverse(
    void *userData,
    QueryCallback branchCallback,
    QueryCallback leafCallback)
{
    vector<tuple<int, Rect, int>> stack;
    stack.emplace_back(ROOT_QUAD_NODE_INDEX, _Bounds.ToRect(), 0);
    while (stack.size() > 0)
    {
        auto [nodeIndex, rect, depth] = stack.back();
        stack.pop_back();

        if (_Nodes.IsLeaf(nodeIndex))
        {
            if (leafCallback != nullptr)
            {
                leafCallback(userData, this, nodeIndex, rect, depth);
            }
        }
        else
        {
            if (branchCallback != nullptr)
            {
                branchCallback(userData, this, nodeIndex, rect, depth);
            }
            const int child = _Nodes.GetChildren(nodeIndex);
            stack.emplace_back(child + 0, rect.TL(), depth + 1);
            stack.emplace_back(child + 1, rect.TR(), depth + 1);
            stack.emplace_back(child + 2, rect.BL(), depth + 1);
            stack.emplace_back(child + 3, rect.BR(), depth + 1);
        }
    }
}

void QuadTree::Clean()
{
    if (_Nodes.IsLeaf(ROOT_QUAD_NODE_INDEX))
    {
        return;
    }

    vector<int> to_process;
    // TODO: Remove magic number
    to_process.reserve(64);
    to_process.push_back(ROOT_QUAD_NODE_INDEX);
    while (to_process.size() > 0)
    {
        const int currentIndex = to_process.back();
        to_process.pop_back();

        // Loop through all the children.
        // count the number of leaves and insert any branch nodes to process
        int empty_child_leaf_count = 0;
        const int child = _Nodes.GetChildren(currentIndex);
        for (int i = 0; i < 4; i++)
        {
            if (_Nodes.IsBranch(child + i))
            {
                to_process.push_back(child + i);
            }
            else
            {
                empty_child_leaf_count += _Nodes.IsEmpty(child + i) ? 1 : 0;
            }
        }

        // If all the child nodes are leaves we can delete this branch
        // node and make it a leaf node again.
        if (empty_child_leaf_count == 4)
        {
            _Nodes.erase(child + 3);
            _Nodes.erase(child + 2);
            _Nodes.erase(child + 1);
            _Nodes.erase(child + 0);
            _Nodes.erase(currentIndex);
            _Nodes.AddLeaf();
        }
    }
}

void QuadTree::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds deltaMs, bool render_rects)
{
    vector<tuple<int, QuadRect, int>> nodes;
    nodes.emplace_back(ROOT_QUAD_NODE_INDEX, this->_Bounds, 0);
    while (nodes.size() > 0)
    {
        auto [currentIndex, currentRect, depth] = nodes.back();
        nodes.pop_back();

        if (_Nodes.IsLeaf(currentIndex))
        {
            if (depth >= 0)
            {
                int alpha = 12 + (64 - depth * (64 / _maxDepth));
                SDL_Rect rect = currentRect.ToSDL(transform);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
                SDL_RenderDrawRect(renderer, &rect);
            }

            if (!render_rects)
            {
                continue;
            }
            int elementNodeIndex = _Nodes.GetChildren(currentIndex);
            while (elementNodeIndex != -1)
            {
                const int elementIndex = _ElementNodes.GetElementId(elementNodeIndex);
                const int left = _Elements.GetLeft(elementIndex);
                const int right = _Elements.GetRight(elementIndex);
                const int top = _Elements.GetTop(elementIndex);
                const int bottom = _Elements.GetBottom(elementIndex);

                Vec2 newPos = transform * Vec2(left, top);
                SDL_Rect rect = {
                    (int)newPos.x,
                    (int)newPos.y,
                    abs(right - left),
                    abs(top - bottom)};
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderDrawRect(renderer, &rect);

                elementNodeIndex = _ElementNodes.GetNext(elementNodeIndex);
            }
        }
        else if (_Nodes.IsBranch(currentIndex))
        {
            const int child = _Nodes.GetChildren(currentIndex);
            nodes.emplace_back(child + 0, currentRect.TL(), depth + 1);
            nodes.emplace_back(child + 1, currentRect.TR(), depth + 1);
            nodes.emplace_back(child + 2, currentRect.BL(), depth + 1);
            nodes.emplace_back(child + 3, currentRect.BR(), depth + 1);
        }
    }
}

// ----------------------------------
// PRIVATE
// ----------------------------------
bool QuadTree::Intersects(
    const int aLeft, const int aTop, const int aRight, const int aBottom,
    const int bLeft, const int bTop, const int bRight, const int bBottom)
{
    return (
        aLeft < bRight &&
        aRight > bLeft &&
        aTop > bBottom &&
        aBottom < bTop);
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

    while (stack.size() > 0)
    {
        const int stack_index = stack.size() - 1;
        const int nd_mx = stack.GetMx(stack_index);
        const int nd_my = stack.GetMy(stack_index);
        const int nd_sx = stack.GetSx(stack_index);
        const int nd_sy = stack.GetSy(stack_index);
        const int currentIndex = stack.GetIndex(stack_index);
        const int depth = stack.GetDepth(stack_index);
        stack.pop_back();

        if (_Nodes.IsLeaf(currentIndex))
        {
            output.Add(currentIndex, depth, nd_mx, nd_my, nd_sx, nd_sy);
        }
        else
        {
            const int child = _Nodes.GetChildren(currentIndex);
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

void QuadTree::InsertNode(int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex)
{
    LeavesListIntList output;
    const int left = _Elements.GetLeft(elementIndex);
    const int right = _Elements.GetRight(elementIndex);
    const int top = _Elements.GetTop(elementIndex);
    const int bottom = _Elements.GetBottom(elementIndex);
    FindLeavesList(
        quadNodeIndex,
        mid_x, mid_y, half_w, half_h,
        depth,
        left, top, right, bottom,
        output);

    for (int i = 0; i < output.size(); i++)
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

void QuadTree::InsertLeafNode(int quadNodeIndex, int mid_x, int mid_y, int half_w, int half_h, int depth, int elementIndex)
{
    // Update the QuadNodes children to the new elementNode
    // Increment the counters
    const int nodeChild = _Nodes.GetChildren(quadNodeIndex);
    const int currentCount = _Nodes.GetCount(quadNodeIndex);
    const int elementNodeIndex = _ElementNodes.Add(elementIndex);
    _ElementNodes.SetNext(elementNodeIndex, nodeChild);
    _Nodes.SetChildren(quadNodeIndex, elementNodeIndex);
    _Nodes.SetCount(quadNodeIndex, currentCount + 1);

    if ((currentCount + 1) >= _splitThreshold && depth < this->_maxDepth)
    {
        // Save the list of elementNodes
        vector<int> tempElementIndices;
        tempElementIndices.reserve(8);

        int tempIndex = elementNodeIndex;
        while (tempIndex != -1)
        {
            int next = _ElementNodes.GetNext(tempIndex);
            int saveElementId = _ElementNodes.GetElementId(tempIndex);
            tempElementIndices.push_back(saveElementId);
            _ElementNodes.erase(tempIndex);
            tempIndex = next;
        }

        // Create the new child QuadNodes
        int tl_index = _Nodes.AddLeaf(); // TL
        _Nodes.AddLeaf();                // TR
        _Nodes.AddLeaf();                // BL
        _Nodes.AddLeaf();                // BR
        _Nodes.MakeBranch(quadNodeIndex, tl_index);

        // Insert the current node's Elements into the new quad nodes
        for (auto tempElementIndex : tempElementIndices)
        {
            InsertNode(quadNodeIndex, mid_x, mid_y, half_w, half_h, depth, tempElementIndex);
        }
    }
}