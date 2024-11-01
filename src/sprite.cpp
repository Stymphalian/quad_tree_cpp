#include <chrono>
#include <SDL_render.h>

#include "sprite.h"
#include "consts.h"
#include "jmath.h"

using namespace std;

Sprite::Sprite(int Id, Vec2 position, Vec2 velocity, Rect BB)
    : _Position(position),
      _Velocity(velocity),
      _BoundingBox(BB),
      _Id(Id),
      _QuadId(-1) {};

void Sprite::Update(Rect &bounds, chrono::milliseconds delta)
{
    _Velocity.Clamp(
        -g_Settings.MaxSpriteVelocity,
        g_Settings.MaxSpriteVelocity);
    _Position += (_Velocity * (float)(delta.count() / 1000.0f));

    if (_Position.x < bounds.L())
    {
        _Position.x = (float)bounds.L();
        _Velocity.x = -_Velocity.x;
    }
    else if (_Position.x > bounds.R())
    {
        _Position.x = (float)bounds.R();
        _Velocity.x = -_Velocity.x;
    }
    if (_Position.y < bounds.B())
    {
        _Position.y = (float)bounds.B();
        _Velocity.y = -_Velocity.y;
    }
    else if (_Position.y > bounds.T())
    {
        _Position.y = (float)bounds.T();
        _Velocity.y = -_Velocity.y;
    }

    _BoundingBox.x = (int)_Position.x;
    _BoundingBox.y = (int)_Position.y;
}

void Sprite::Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds deltaMs)
{
    Vec2 newPos = transform * _Position;
    SDL_Rect rect = {
        (int)newPos.x - _BoundingBox.W2(),
        (int)newPos.y - _BoundingBox.H2(),
        _BoundingBox.w,
        _BoundingBox.h};

    if (_IsColliding)
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

bool Sprite::Intersects(Sprite *other)
{
    return _BoundingBox.Intersects(other->_BoundingBox);
}

void Sprite::Collides(Sprite *B)
{
    Sprite *A = this;
    Vec2 dir = (B->_Position - A->_Position).Normalize(); // A to B
    Vec2 rdir = dir * -1.0;
    Vec2 dirNorm = dir.GetNormal();
    Vec2 rDirNorm = dirNorm * -1.0;

    Vec2 compDirA = dir * A->_Velocity.dot(dir);
    Vec2 compDirB = rdir * B->_Velocity.dot(rdir);
    Vec2 compNormA = dirNorm * A->_Velocity.dot(dirNorm);
    Vec2 compNormB = rDirNorm * B->_Velocity.dot(rDirNorm);

    // TODO: Extremely scuff collission resolution logic.
    A->_Velocity = compNormA + compDirB;
    A->_Position = A->_Position - dir;
    B->_Velocity = compNormB + compDirA;
    B->_Position = B->_Position - rdir;
}
