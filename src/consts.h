#pragma once

// 14825 FPS quad_tree_c (10000x10000 2000 sprites, (1,50), 16, 8, 400x400)
// 80 FPS my_implementation
// 1600 FPS by removing metrics collection
// 6500 FPS by using int_list (same data structre as quad_tree_c)

struct GlobalSettings {
    const int WorldWidth = 10000;
    const int WorldHeight = 10000;
    const int NumberSprites = 20000;
    const int MaxSpriteVelocity = 120;
    const int MinRectSize = 5;
    const int MaxRectSize = 10;
    const int MaxQuadTreeDepth = 16;
    const int QuadTreeSplitThreshold = 8;
    const bool UseQuadTree = true;
    const int ViewportWidth = 400;
    const int ViewportHeight = 400;
};

static const GlobalSettings g_Settings;