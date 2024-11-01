#pragma once
#include <chrono>
#include <SDL_render.h>

#include "consts.h"
#include "jmath.h"

class Sprite
{
public:
    Vec2 _Position;
    Vec2 _Velocity;
    Rect _BoundingBox;
    int _Id;
    int _QuadId;
    bool _IsColliding = false;

public:
    Sprite(int Id, Vec2 position, Vec2 velocity, Rect BB);
    ~Sprite() = default;

    void Update(Rect &bounds, std::chrono::milliseconds delta);
    void Draw(SDL_Renderer *renderer,
              Mat3 &transform,
              std::chrono::milliseconds deltaMs);
    bool Intersects(Sprite *other);
    void Collides(Sprite *B);
};