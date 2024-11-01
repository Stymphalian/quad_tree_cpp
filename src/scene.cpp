#include "scene.h"

#include "jmath.h"
#include "jquad.h"
#include "consts.h"

Scene::Scene(Rect BB)
    : _WorldBox(BB),
      _QuadTree(
          BB,
          g_Settings.MaxQuadTreeDepth,
          g_Settings.QuadTreeSplitThreshold) {}
Scene::~Scene() {}

void Scene::Build()
{
    for (Sprite &sprite : _Sprites)
    {
        sprite._QuadId = _QuadTree.Insert(sprite._Id, sprite._BoundingBox);
    }
}

struct QuadCollisionUserData
{
    unordered_map<int, bool> seen;
    vector<bool> spritesProcessed;
    vector<pair<int, int>> collisionPairs;
    Scene *scene;
};

void quadCollissionLeafCallback(
    void *userData,
    QuadTree *tree,
    int nodeIndex,
    Rect nodeRect,
    int depth)
{
    QuadCollisionUserData *data = (QuadCollisionUserData *)userData;
    Scene *scene = data->scene;
    unordered_map<int, bool> &seen = data->seen;
    vector<bool> &traversed = data->spritesProcessed;
    vector<pair<int, int>> &collissionPairs = data->collisionPairs;

    vector<int> intersectingElements;
    int elementNodeIndex = tree->_Nodes.GetChildren(nodeIndex);
    // Process every element in this node
    while (elementNodeIndex != -1)
    {
        const int elementIndex = tree->_ElementNodes.GetElementId(elementNodeIndex);
        elementNodeIndex = tree->_ElementNodes.GetNext(elementNodeIndex);

        // If we have already processed this element then we can skip early.
        // This can happen from processing in a separate leaf node
        if (traversed[elementIndex])
        {
            continue;
        }

        const Rect queryRect = std::move(tree->_Elements.GetRect(elementIndex));
        traversed[elementIndex] = true;

        seen.clear();
        intersectingElements.clear();
        seen[elementIndex] = true;
        tree->Query(queryRect, seen, &intersectingElements);

        const int A_spriteId = tree->_Elements.GetId(elementIndex);
        for (int otherElementId : intersectingElements)
        {
            const int B_spriteId = tree->_Elements.GetId(otherElementId);
            if (traversed[otherElementId])
            {
                continue;
            }
            collissionPairs.emplace_back(A_spriteId, B_spriteId);
        }
    }
};

void Scene::QuadCollision()
{
    QuadCollisionUserData data;
    data.scene = this;
    // TODO: Remove this magic number.
    data.collisionPairs.reserve(4096);
    data.spritesProcessed.resize(g_Settings.NumberSprites, false);
    _QuadTree.Traverse((void *)(&data), nullptr, quadCollissionLeafCallback);

    Sprite *A = nullptr;
    Sprite *B = nullptr;
    for (auto [AId, BId] : data.collisionPairs)
    {
        A = &_Sprites[AId];
        B = &_Sprites[BId];
        A->Collides(B);
        A->_IsColliding = true;
        B->_IsColliding = true;
    }
}

void Scene::BruteCollision()
{
    // Run collision logic
    Sprite *A = nullptr;
    Sprite *B = nullptr;
    for (int i = 0; i < _Sprites.size(); i++)
    {
        A = &_Sprites[i];
        for (int j = i + 1; j < _Sprites.size(); j++)
        {
            B = &_Sprites[j];
            if (A->Intersects(B) && B->Intersects(A))
            {
                A->_IsColliding = true;
                B->_IsColliding = true;
                _Sprites[i].Collides(&_Sprites[j]);
            }
        }
    }
}

void Scene::Update(chrono::milliseconds deltaMs)
{
    for (Sprite &sprite : _Sprites)
    {
        sprite._IsColliding = false;
    }

    if (g_Settings.UseQuadTree)
    {
        QuadCollision();
    }
    else
    {
        BruteCollision();
    }

    // update physics
    for (Sprite &sprite : _Sprites)
    {
        sprite.Update(_WorldBox, deltaMs);
        _QuadTree.Remove(sprite._QuadId);
        sprite._QuadId = _QuadTree.Insert(sprite._Id, sprite._BoundingBox);
    }
}

void Scene::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds deltaMs)
{
    _QuadTree.Draw(renderer, transform, deltaMs, _DrawQuadTreeRects);

    if (_DrawSpriteRects)
    {
        for (Sprite &sprite : _Sprites)
        {
            sprite.Draw(renderer, transform, deltaMs);
        }
    }
}

void Scene::Clean()
{
    _QuadTree.Clean();
}
