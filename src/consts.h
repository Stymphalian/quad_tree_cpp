#pragma once

// 14825 FPS quad_tree_c
// 80 FPS my_implementation
// 1600 FPS by removing metrics collection
const int WORLD_WIDTH = 10000;
const int WORLD_HEIGHT = 10000;
const int NUMBER_SPRITES = 10000;
const int MAX_SPRITE_VELOCITY = 120;
const int MIN_RECT_SIZE = 5;
const int MAX_RECT_SIZE = 50;
const int MAX_QUAD_TREE_DEPTH = 16;
const int QUAD_TREE_SPLIT_THRESHOLD = 8;
const bool USE_QUAD_TREE = true;
const int VIEWPORT_WIDTH = 400;
const int VIEWPORT_HEIGHT = 400;

// const int WORLD_WIDTH = 400;
// const int WORLD_HEIGHT = 400;
// const int NUMBER_SPRITES = 10;
// const int MAX_SPRITE_VELOCITY = 500;
// const int MIN_RECT_SIZE = 5;
// const int MAX_RECT_SIZE = 50;
// const int MAX_QUAD_TREE_DEPTH = 4;
// const int QUAD_TREE_SPLIT_THRESHOLD = 3;
// const bool USE_QUAD_TREE = true;
// const int VIEWPORT_WIDTH = 400;
// const int VIEWPORT_HEIGHT = 400;

// Should be 2000 FPS with no collisions detection
// const int WORLD_WIDTH = 10000;
// const int WORLD_HEIGHT = 10000;
// const int NUMBER_SPRITES = 2000;
// const int MAX_SPRITE_VELOCITY = 120;
// const int MIN_RECT_SIZE = 5;
// const int MAX_RECT_SIZE = 10;
// const int MAX_QUAD_TREE_DEPTH = 16;
// const int QUAD_TREE_SPLIT_THRESHOLD = 8;
// const bool USE_QUAD_TREE = true;
// const int VIEWPORT_WIDTH = 400;
// const int VIEWPORT_HEIGHT = 400;