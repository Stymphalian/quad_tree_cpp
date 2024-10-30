#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <string>
#include <chrono>
#include <iostream>
#include <tuple>
#include <cassert>
#include <SDL_render.h>

#include "jmath.h"
#include "jquad.h"
#include "metrics.h"
#include "consts.h"

class Sprite
{
public:
    Vec2 Position;
    Vec2 Velocity;
    Rect BoundingBox;
    double Mass = 1.0;
    int Id;
    int QuadId;
    bool IsColliding = false;
    // unordered_map<int, bool> already_collided;

    Sprite(int Id, Vec2 position, Vec2 velocity, Rect BB)
    {
        this->Position = position;
        this->Velocity = velocity;
        this->BoundingBox = BB;
        this->Id = Id;
        this->QuadId = -1;
    };
    ~Sprite() = default;

    void Update(Rect &bounds, chrono::milliseconds delta)
    {
        // already_collided.clear();

        // this->Velocity.Clamp(-MAX_SPRITE_VELOCITY, MAX_SPRITE_VELOCITY);
        this->Position += (this->Velocity * (float)(delta.count() / 1000.0f));

        if (this->Position.x < bounds.L())
        {
            this->Position.x = (float)bounds.L();
            this->Velocity.x = -this->Velocity.x;
        }
        else if (this->Position.x > bounds.R())
        {
            this->Position.x = (float)bounds.R();
            this->Velocity.x = -this->Velocity.x;
        }
        if (this->Position.y < bounds.B())
        {
            this->Position.y = (float)bounds.B();
            this->Velocity.y = -this->Velocity.y;
        }
        else if (this->Position.y > bounds.T())
        {
            this->Position.y = (float)bounds.T();
            this->Velocity.y = -this->Velocity.y;
        }

        this->BoundingBox.x = (int)this->Position.x;
        this->BoundingBox.y = (int)this->Position.y;
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        Vec2 newPos = transform * this->Position;
        SDL_Rect rect = {
            (int)newPos.x - this->BoundingBox.W2(),
            (int)newPos.y - this->BoundingBox.H2(),
            this->BoundingBox.w,
            this->BoundingBox.h};

        if (this->IsColliding)
        {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
        SDL_RenderDrawPoint(renderer, (int)newPos.x, (int)newPos.y);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool Intersects(Sprite *other)
    {
        return (this->Position - other->Position).Length() < (this->BoundingBox.w + other->BoundingBox.w) / 2;
        // return this->BoundingBox.Intersects(other->BoundingBox);
    }

    void Collides(Sprite *B)
    {
        Sprite *A = this;
        Vec2 dir = (B->Position - A->Position).Normalize(); // A to B
        Vec2 rdir = dir * -1.0;
        Vec2 dir_norm = dir.GetNormal();
        Vec2 rdir_norm = dir * -1.0;

        Vec2 comp_dir_a = dir * A->Velocity.dot(dir);
        Vec2 comp_dir_b = rdir * B->Velocity.dot(rdir);
        Vec2 comp_norm_a = dir_norm * A->Velocity.dot(dir_norm);
        Vec2 comp_norm_b = rdir_norm * B->Velocity.dot(rdir_norm);

        A->Velocity = comp_norm_a + comp_dir_b;
        A->Position = A->Position - dir;
        // B->Velocity = comp_norm_b + comp_dir_a;
        // B->Position = B->Position - rdir;
    }
};

class Scene
{
public:
    QuadTree quadTree;
    vector<Sprite> Sprites;
    Rect WorldBox;
    bool draw_quad_tree_rects = false;
    bool draw_sprite_rects = true;

    Scene(Rect BB);
    ~Scene();

    void Build();
    void QuadCollision();
    void BruteCollision();

    void Update(chrono::milliseconds delta_ms);
    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms);
    void Clean();
};
