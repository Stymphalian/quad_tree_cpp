#define SDL_MAIN_HANDLED
#include "SDL.h"

#include "jmath.h"
#include "consts.h"
#include "jquad.h"
#include "scene.h"

using namespace std;
typedef std::chrono::high_resolution_clock rclock;

class Game
{
public:
    int _width;
    int _height;
    Rect _WorldBox;
    Scene _Scene;

public:
    Game(int width, int height)
        : _width(width),
          _height(height),
          _WorldBox(0, 0, width, height),
          _Scene(_WorldBox)
    {
        // Initialize random seed
        srand(1250);

        printf("World Width = %d, World Height = %d\n", width, height);
        printf("Max Depth = %d\n", g_Settings.MaxQuadTreeDepth);
        printf("Max Rect Size = %d\n", g_Settings.MaxRectSize);

        // Create the scene
        const int numberSprites = g_Settings.NumberSprites;
        const int maxSpritVelocity = g_Settings.MaxSpriteVelocity;
        _Scene._Sprites.reserve(numberSprites);
        for (int i = 0; i < numberSprites; ++i)
        {
            float posX = (float)(rand() % _WorldBox.W2()) - _WorldBox.W4();
            float posY = (float)(rand() % _WorldBox.H2()) - _WorldBox.H4();
            float velX = (float)((rand() % 1000) / 1000.0) * maxSpritVelocity - maxSpritVelocity / 2;
            float velY = (float)((rand() % 1000) / 1000.0) * maxSpritVelocity - maxSpritVelocity / 2;
            int w = g_Settings.MinRectSize + (rand() % g_Settings.MaxRectSize);
            // int h = g_Settings.MinRectSize + (rand() % g_Settings.MaxRectSize);
            int h = w;

            Rect bounds = Rect((int)posX, (int)posY, w, h);
            _Scene._Sprites.emplace_back(
                i,
                Vec2(posX, posY),
                Vec2(velX, velY),
                bounds);
        }
        _Scene.Build();
    }

    ~Game() = default;

    void UpdatePhysics(chrono::milliseconds deltaMs)
    {
        _Scene.Update(deltaMs);
    }

    void Draw(SDL_Renderer *renderer, Mat3 &transform, chrono::milliseconds deltaMs)
    {
        _Scene.Draw(renderer, transform, deltaMs);
        _Scene.Clean();
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
        g_Settings.ViewportWidth,
        g_Settings.ViewportHeight,
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

    Game game(g_Settings.WorldWidth, g_Settings.WorldHeight);
    bool quit = false;
    bool paused = false;
    auto start = rclock::now();

    Mat3 transform;
    transform = Mat3::Scale(1, -1) * Mat3::Translate(
                                         g_Settings.ViewportWidth / 2,
                                         g_Settings.ViewportHeight / 2);

    int frameCount = 0;
    double totalTimeSec = 0;
    while (!quit)
    {
        auto frameStartTime = rclock::now();

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
                    game._Scene._DrawQuadTreeRects = !game._Scene._DrawQuadTreeRects;
                    break;
                case SDLK_r:
                    game._Scene._DrawSpriteRects = !game._Scene._DrawSpriteRects;
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
        auto deltaMs = chrono::duration_cast<chrono::milliseconds>(now - start);

        // Update Physics
        if (deltaMs >= chrono::milliseconds(16))
        {
            start = now;
            if (!paused)
            {
                game.UpdatePhysics(deltaMs);
            }
        }

        // Draw Stuff
        game.Draw(renderer, transform, deltaMs);

        // Draw origin
        Vec2 origin = transform * Vec2(0, 0);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_Rect rr;
        rr.x = (int)origin.x - 5;
        rr.y = (int)origin.y - 5;
        rr.w = 10;
        rr.h = 10;
        SDL_RenderDrawRect(renderer, &rr);
        SDL_RenderDrawPoint(renderer, (int)origin.x, (int)origin.y);

        SDL_RenderPresent(renderer);

        auto frameEndTime = rclock::now();
        auto frameDeltaMs = chrono::duration_cast<chrono::milliseconds>(frameEndTime - frameStartTime);
        totalTimeSec += (frameDeltaMs.count() / 1000.0);
        frameCount++;
        if (frameCount % 60 == 0)
        {
            printf("Frame Rate: %.2f fps\n", frameCount / totalTimeSec);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}