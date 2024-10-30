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
        // auto start_time = rclock::now();
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
        B->Velocity = comp_norm_b + comp_dir_a;
        A->Position = A->Position - dir;
        B->Position = B->Position - rdir;
        // auto end_time = rclock::now();
        // METRICS.RecordSpriteCollide(end_time - start_time);
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

    Scene(Rect BB) : WorldBox(BB),
                     quadTree(BB, MAX_QUAD_TREE_DEPTH, QUAD_TREE_SPLIT_THRESHOLD)
    {
    }
    ~Scene()
    {
    }

    void Build()
    {
        for (Sprite &sprite : Sprites)
        {
            int quadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
            sprite.QuadId = quadId;
        }
    }

    void QuadCollision()
    {
        // auto start_time = rclock::now();

        // std::function<Sprite *(int id)> getDataFn = [&](int id) -> Sprite *
        // {
        //     return &Sprites[id];
        // };

        // unordered_map<int, bool> sprite_processed;
        // std::function<void(Sprite * sprite)> useDataFn = [this, getDataFn, &sprite_processed](Sprite *A)
        // {
        //     if (sprite_processed.count(A->Id) != 0)
        //     {
        //         return;
        //     }

        //     unordered_map<int, bool> child_processed;
        //     std::function<void(Sprite * sprite)> useDataFn = [this, &sprite_processed, &child_processed, A](Sprite *B)
        //     {
        //         if (A->Id == B->Id
        //             || child_processed.count(B->Id) != 0
        //             || sprite_processed.count(B->Id) != 0) {
        //             return;
        //         }

        //         if (A->Intersects(B))
        //         {
        //             A->IsColliding = true;
        //             B->IsColliding = true;
        //             A->Collides(B);
        //             child_processed[B->Id] = true;
        //         }
        //     };
        //     quadTree.QueryWithData(
        //         A->BoundingBox,
        //         getDataFn,
        //         useDataFn
        //     );
        //     sprite_processed[A->Id] = true;
        // };
        // quadTree.QueryWithData(
        //     WorldBox,
        //     getDataFn,
        //     useDataFn
        // );

        // auto end_time = rclock::now();
        // METRICS.RecordQuadCollision(end_time - start_time);
    }

    void BruteCollision()
    {
        // Run collision logic
        Sprite *A = nullptr;
        Sprite *B = nullptr;
        for (int i = 0; i < Sprites.size(); i++)
        {
            A = &Sprites[i];
            for (int j = i + 1; j < Sprites.size(); j++)
            {
                // if ( i == j) {continue;}
                B = &Sprites[j];
                if (A->Intersects(B) && B->Intersects(A))
                {
                    A->IsColliding = true;
                    B->IsColliding = true;
                    Sprites[i].Collides(&Sprites[j]);
                }
            }
        }
    }

    void Update(chrono::milliseconds delta_ms)
    {
        for (Sprite &sprite : Sprites)
        {
            sprite.IsColliding = false;
        }

        // if (USE_QUAD_TREE)
        // {
        //     QuadCollision();
        // }
        // else
        // {
        //     BruteCollision();
        // }

        // Update the physics
        // auto start_time = rclock::now();
        for (Sprite &sprite : Sprites)
        {
            sprite.Update(WorldBox, delta_ms);
        }
        // auto end_time = rclock::now();
        // METRICS.RecordSpriteUpdatePhyscis(end_time - start_time);

        // start_time = rclock::now();
        for (Sprite &sprite : Sprites)
        {
            quadTree.Remove(sprite.QuadId);
            sprite.QuadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        }
        // end_time = rclock::now();
        // METRICS.RecordSpriteQuadTreeUpdate(end_time - start_time);
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        // auto start_time = rclock::now();
        // Draw the quad tree
        quadTree.Draw(renderer, transform, delta_ms);

        if (draw_sprite_rects)
        {
            for (Sprite &sprite : Sprites)
            {
                sprite.Draw(renderer, transform, delta_ms);
            }
        }

        // auto end_time = rclock::now();
        // METRICS.RecordSceneDraw(end_time - start_time);
    }

    void Clean()
    {
        quadTree.Clean();
    }
};
