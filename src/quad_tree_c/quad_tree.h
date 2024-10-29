#ifndef QUADTREE_H
#define QUADTREE_H

#include "IntList.h"

#ifdef __cplusplus
    #define QTREE_FUNC extern "C"
#else
    #define QTREE_FUNC
#endif

typedef struct Quadtree Quadtree;

struct Quadtree
{
    // Stores all the nodes in the quadtree. The first node in this
    // sequence is always the root.
    IntList nodes;

    // Stores all the elements in the quadtree.
    IntList elts;

    // Stores all the element nodes in the quadtree.
    IntList enodes;

    // Stores the quadtree extents.
    int root_mx, root_my, root_sx, root_sy;

    // Maximum allowed elements in a leaf before the leaf is subdivided/split unless
    // the leaf is at the maximum allowed tree depth.
    int max_elements;

    // Stores the maximum depth allowed for the quadtree.
    int max_depth;

    // Temporary buffer used for queries.
    char* temp;

    // Stores the size of the temporary buffer.
    int temp_size;
};


enum
{
    // ----------------------------------------------------------------------------------------
    // Element node fields:
    // ----------------------------------------------------------------------------------------
    enode_num = 2,

    // Points to the next element in the leaf node. A value of -1 
    // indicates the end of the list.
    enode_idx_next = 0,

    // Stores the element index.
    enode_idx_elt = 1,

    // ----------------------------------------------------------------------------------------
    // Element fields:
    // ----------------------------------------------------------------------------------------
    elt_num = 5,

    // Stores the rectangle encompassing the element.
    elt_idx_lft = 0, elt_idx_top = 1, elt_idx_rgt = 2, elt_idx_btm = 3,

    // Stores the ID of the element.
    elt_idx_id = 4,

    // ----------------------------------------------------------------------------------------
    // Node fields:
    // ----------------------------------------------------------------------------------------
    node_num = 2,

    // Points to the first child if this node is a branch or the first element
    // if this node is a leaf.
    node_idx_fc = 0,

    // Stores the number of elements in the node or -1 if it is not a leaf.
    node_idx_num = 1,

    // ----------------------------------------------------------------------------------------
    // Node data fields:
    // ----------------------------------------------------------------------------------------
    nd_num = 6,

    // Stores the extents of the node using a centered rectangle and half-size.
    nd_idx_mx = 0, nd_idx_my = 1, nd_idx_sx = 2, nd_idx_sy = 3,

    // Stores the index of the node.
    nd_idx_index = 4,

    // Stores the depth of the node.
    nd_idx_depth = 5,
};

// Function signature used for traversing a tree node.
typedef void QtNodeFunc(Quadtree* qt, void* user_data, int node, int depth, int mx, int my, int sx, int sy);

// Creates a quadtree with the requested extents, maximum elements per leaf, and maximum tree depth.
QTREE_FUNC void qt_create(Quadtree* qt, int width, int height, int max_elements, int max_depth);

// Destroys the quadtree.
QTREE_FUNC void qt_destroy(Quadtree* qt);

// Inserts a new element to the tree.
// Returns an index to the new element.
QTREE_FUNC int qt_insert(Quadtree* qt, int id, float x1, float y1, float x2, float y2);

// Removes the specified element from the tree.
QTREE_FUNC void qt_remove(Quadtree* qt, int element);

// Cleans up the tree, removing empty leaves.
QTREE_FUNC void qt_cleanup(Quadtree* qt);

// Outputs a list of elements found in the specified rectangle.
QTREE_FUNC void qt_query(Quadtree* qt, IntList* out, float x1, float y1, float x2, float y2, int omit_element);

// Traverses all the nodes in the tree, calling 'branch' for branch nodes and 'leaf' 
// for leaf nodes.
QTREE_FUNC void qt_traverse(Quadtree* qt, void* user_data, QtNodeFunc* branch, QtNodeFunc* leaf);

#endif
