#include "scene.h"

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

Scene::Scene(Rect BB) : WorldBox(BB),
                        quadTree(BB, MAX_QUAD_TREE_DEPTH, QUAD_TREE_SPLIT_THRESHOLD)
{
}
Scene::~Scene()
{
}

void Scene::Build()
{
    for (Sprite &sprite : Sprites)
    {
        int quadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
        sprite.QuadId = quadId;
    }
}


struct QuadCollisionUserData {
    unordered_map<int, bool> seen;
    unordered_map<int, bool> spritesProcessed;
    Scene* scene;
};
void Scene::QuadCollision()
{
    QuadTree::QueryCallback callback = [](
                                           void *user_data,
                                           QuadTree *tree,
                                           int nodeIndex,
                                           Rect nodeRect,
                                           int depth)
    {
        QuadCollisionUserData *data = (QuadCollisionUserData*)user_data;
        Scene *scene = data->scene;
        unordered_map<int, bool> &seen = data->seen;
        unordered_map<int, bool> &traversed = data->spritesProcessed;

        vector<int> intersectingElements;
        int elementNodeIndex = tree->Nodes.GetChildren(nodeIndex);
        // Process every element in this node
        while (elementNodeIndex != -1)
        {
            const int elementIndex = tree->ElementNodes.GetElementId(elementNodeIndex);
            elementNodeIndex = tree->ElementNodes.GetNext(elementNodeIndex);

            // If we have already processed this element (from processing 
            // a separate leaf node), then we can skip early.
            if (traversed.find(elementIndex) != traversed.end())
            {   
                continue;
            }

            const int A_spriteId = tree->Elements.GetId(elementIndex);
            Sprite* A = &(scene->Sprites[A_spriteId]);
            traversed[elementIndex] = true;

            seen.clear();
            intersectingElements.clear();
            seen[elementIndex] = true;
            tree->Query(A->BoundingBox, seen, &intersectingElements);

            for (int otherElementId: intersectingElements)
            {
                const int B_spriteId = tree->Elements.GetId(otherElementId);
                Sprite* B = &(scene->Sprites[B_spriteId]);
                assert(A->Id != B->Id);

                if (traversed.find(otherElementId) != traversed.end())
                {
                    continue;
                }                
                A->Collides(B);
                A->IsColliding = true;
            }
        }
    };

    QuadCollisionUserData data;
    data.scene = this;
    // quadTree.Traverse((void*)(&data), nullptr, callback);
}

void Scene::BruteCollision()
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

void Scene::Update(chrono::milliseconds delta_ms)
{
    for (Sprite &sprite : Sprites)
    {
        sprite.IsColliding = false;
    }

    if (USE_QUAD_TREE)
    {
        QuadCollision();
    }
    else
    {
        BruteCollision();
    }

    // update physics
    for (Sprite &sprite : Sprites)
    {
        sprite.Update(WorldBox, delta_ms);
        quadTree.Remove(sprite.QuadId);
        sprite.QuadId = quadTree.Insert(sprite.Id, sprite.BoundingBox);
    }
}

void Scene::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
{
    // auto start_time = rclock::now();
    // Draw the quad tree
    quadTree.Draw(renderer, transform, delta_ms, draw_quad_tree_rects);

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

void Scene::Clean()
{
    quadTree.Clean();
}
