// To build on Ubuntu:
// sudo apt install libsdl2-dev

#include <iostream>
#include <SDL2/SDL.h>

// Note: SDL documentation for each function name at:
// https://wiki.libsdl.org/SDL_CreateRenderer
//                         ^^^^^^^^^^^^^^^^^^ function name

int main(int /*argc*/, char **/*argv*/)
{
    SDL_Init(SDL_INIT_VIDEO);

    const int winWidth = 640;
    const int winHeight = 480;
    auto win = SDL_CreateWindow("framebuffer", SDL_WINDOWPOS_CENTERED | SDL_WINDOW_OPENGL, SDL_WINDOWPOS_CENTERED, winWidth, winHeight, 0);
    auto renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    while(true)
    {
        SDL_Event e;
        if(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT)
            {
                break;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawPoint(renderer, 100/* x */, 100/* y */);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);

    SDL_Quit();

    return 0;
}
