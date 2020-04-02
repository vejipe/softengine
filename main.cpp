// To build on Ubuntu:
// sudo apt install libsdl2-dev

#include <stdint.h>

// SDL includes:
#include <SDL2/SDL.h>

// Note: SDL documentation for each function name at:
// https://wiki.libsdl.org/SDL_CreateRenderer
//                         ^^^^^^^^^^^^^^^^^^ function name

// Inspired from:
// https://www.davrous.com/2013/06/13/tutorial-series-learning-how-to-write-a-3d-soft-engine-from-scratch-in-c-typescript-or-javascript/

struct color4
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

class Device
{
public:
    Device(const int winWidth, const int winHeight)
        : m_winWidth(winWidth)
        , m_winHeight(winHeight)
        , m_window( SDL_CreateWindow(
              "framebuffer",
              SDL_WINDOWPOS_CENTERED | SDL_WINDOW_OPENGL,
              SDL_WINDOWPOS_CENTERED,
              m_winWidth, m_winHeight, 0) )
        , m_renderer( SDL_CreateRenderer(
              m_window, -1, SDL_RENDERER_ACCELERATED) )
    { }

    ~Device()
    {
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
    }

    void clear(color4 c)
    {
        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderClear(m_renderer);
    }

    void present()
    {
        SDL_RenderPresent(m_renderer);
    }

    void drawPoint(int16_t x, int16_t y, color4 c)
    {
        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderDrawPoint(m_renderer, x, y);
    }

private:
    const uint16_t m_winWidth;
    const uint16_t m_winHeight;

private:
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
};

int main(int /*argc*/, char **/*argv*/)
{
    SDL_Init(SDL_INIT_VIDEO);

    Device device(640, 480);

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

        device.clear({0, 0, 0, 255});
        device.drawPoint(100, 100, {255, 255, 255, 255});
        device.present();
    }

    SDL_Quit();

    return 0;
}
