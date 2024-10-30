
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

#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "free_list.hpp"
#include "jmath.h"
#include "consts.h"
#include "metrics.h"
#include "jquad.h"
#include "scene.hpp"

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

class Game
{
public:
    int width;
    int height;
    Rect WorldBox;
    Scene scene;

    Game(): width(WORLD_WIDTH), height(WORLD_HEIGHT), WorldBox(0, 0, width, height), scene(WorldBox)
    {
        // Initialize random seed
        srand(1250);

        printf("World Width = %d, World Height = %d\n", WORLD_WIDTH, WORLD_HEIGHT);
        printf("Max Depth = %d\n", MAX_QUAD_TREE_DEPTH);
        printf("Max Rect Size = %d\n", MAX_RECT_SIZE);

        // Create the scene
        scene.Sprites.reserve(NUMBER_SPRITES);
        for (int i = 0; i < NUMBER_SPRITES; ++i)
        {
            float posX = (float)(rand() % WorldBox.W2()) - WorldBox.W4();
            float posY = (float)(rand() % WorldBox.H2()) - WorldBox.H4();
            // float posX = (float)(rand() % WorldBox.W2()) + (WorldBox.W4() >>1);
            // float posY = (float)(rand() % WorldBox.W2()) + (WorldBox.H4() >>1);
            float velX = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            float velY = (float)((rand() % 1000) / 1000.0) * MAX_SPRITE_VELOCITY - MAX_SPRITE_VELOCITY / 2;
            int w = MIN_RECT_SIZE + (rand() % MAX_RECT_SIZE);
            // int h = MIN_RECT_SIZE + (rand() % MAX_RECT_SIZE);
            int h = w;

            Rect bounds = Rect((int)posX, (int)posY, w, h);
            scene.Sprites.push_back(
                Sprite(i,
                       Vec2(posX, posY),
                       Vec2(velX, velY),
                       bounds));
        }
        scene.Build();
    }

    ~Game() = default;

    void UpdatePhysics(chrono::milliseconds delta_ms)
    {
        scene.Update(delta_ms);
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds delta_ms)
    {
        scene.Draw(renderer, transform, delta_ms);
        scene.Clean();
    }
};

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Quad Trees",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        VIEWPORT_WIDTH,
        VIEWPORT_HEIGHT,
        0);
    if (!window)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    Game game;
    bool quit = false;
    bool paused = false;
    auto start = rclock::now();

    Mat3 transform;
    // transform = Mat3::Scale(1, -1) * Mat3::Translate(-WORLD_WIDTH/2, WORLD_HEIGHT/2);
    transform = Mat3::Scale(1, -1) * Mat3::Translate(VIEWPORT_WIDTH/2, VIEWPORT_HEIGHT/2);

    auto game_start_time = rclock::now();

    int frame_count = 0;
    double total_time_sec = 0;
    while (!quit)
    {
        auto frame_start_time = rclock::now();

        SDL_Event e;
        // Handle input
        while (SDL_PollEvent(&e) != 0)
        {
            switch (e.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                switch (e.key.keysym.sym)
                {
                case SDLK_w:
                    transform = Mat3::Translate(0, -10) * transform;
                    break;
                case SDLK_s:
                    transform = Mat3::Translate(0, 10) * transform;
                    break;
                case SDLK_a:
                    transform = Mat3::Translate(10, 0) * transform;
                    break;
                case SDLK_d:
                    transform = Mat3::Translate(-10, 0) * transform;
                    break;
                default:
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.sym)
                {
                case SDLK_SPACE:
                    paused = !paused;
                    break;
                case SDLK_f:
                    game.scene.draw_quad_tree_rects = !game.scene.draw_quad_tree_rects;
                    break;
                case SDLK_r:
                    game.scene.draw_sprite_rects = !game.scene.draw_sprite_rects;
                    break;
                }
            default:
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_RenderClear(renderer);
        auto now = rclock::now();
        auto delta_ms = chrono::duration_cast<chrono::milliseconds>(now - start);

        // Update Physics
        if (delta_ms >= chrono::milliseconds(16))
        {
            start = now;
            if (!paused)
            {
                game.UpdatePhysics(delta_ms);
            }
        }

        // Draw Stuff
        game.Draw(renderer, transform, delta_ms);

        // Draw origin
        Vec2 origin = transform * Vec2(WORLD_WIDTH/2, WORLD_HEIGHT/2);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_Rect rr;
        rr.x = (int)origin.x;
        rr.y =  (int)origin.y;
        rr.w = 100;
        rr.h = 100;
        SDL_RenderDrawRect(renderer, &rr);
        SDL_RenderDrawPoint(renderer, (int)origin.x, (int)origin.y);

        SDL_RenderPresent(renderer);

        auto frame_end_time = rclock::now();
        auto frame_delta_ms = chrono::duration_cast<chrono::milliseconds>(frame_end_time - frame_start_time);
        total_time_sec += (frame_delta_ms.count() / 1000.0);
        frame_count++;
        if (frame_count % 60 == 0)
        {
            printf("Frame Rate: %.2f fps\n", frame_count / total_time_sec);
        }
    }

    auto game_end_time = rclock::now();
    METRICS.RecordTotalTime(game_end_time - game_start_time);
    METRICS.Print();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}