# Quad Tree
Quad tree implementation in C++. \
My goal for this project was to (re)learn C++ and implementing data structures is always a fun way to do this.

Lots of code was inspired/copied from this [SO post](https://stackoverflow.com/questions/41946007/efficient-and-well-explained-implementation-of-a-quadtree-for-2d-collision-det) which provided lots of interesting
ideas for optimizing the data structure.
I'm not even close to the performance described in the post (200k elements at 60fps)
but I'm too lazy to figure out further optimizations for it.

The program is an SDL application which shows a simulation of AABB rects with
elastic collisions just bouncing around. Using QuadTrees is not ideal for this
type of application (due to the objects constantly moving around requiring constant updates to the tree).
but it does really push the limits of the structure.

