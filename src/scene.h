#pragma once

#include <vector>
#include <unordered_map>

#include "consts.h"
#include "jmath.h"
#include "jquad.h"
#include "sprite.h"

class Scene
{
public:
    QuadTree _QuadTree;
    vector<Sprite> _Sprites;

    Rect _WorldBox;
    bool _DrawQuadTreeRects = false;
    bool _DrawSpriteRects = true;

public:
    Scene(Rect BB);
    ~Scene();

    void Build();
    void QuadCollision();
    void BruteCollision();

    void Update(chrono::milliseconds deltaMs);
    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds deltaMs);
    void Clean();
};
