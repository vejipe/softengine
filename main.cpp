// ### To build on Ubuntu:
// # For SDL2 devel (Simple DirectMedia Library)
// # http://www.libsdl.org
// sudo apt install libsdl2-dev
// # For GLM devel header (OpenGL Mathematics)
// # https://glm.g-truc.net
// sudo apt install libglm-dev

#include <stdint.h>

// GLM includes:
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
// glm::translate, glm::rotate, glm::perspective

// Libstd includes;
#include <cmath> // std::abs
#include <array>
#include <vector>

// SDL includes:
#include <SDL2/SDL.h>

// Note: SDL documentation for each function name at:
// https://wiki.libsdl.org/SDL_CreateRenderer
//                         ^^^^^^^^^^^^^^^^^^ function name

// Inspired from:
// https://www.davrous.com/2013/06/13/tutorial-series-learning-how-to-write-a-3d-soft-engine-from-scratch-in-c-typescript-or-javascript/
// https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/

// RGBA_8888 = 32 bits / pixel
struct color4
{
    uint8_t r;  // red
    uint8_t g;  // green
    uint8_t b;  // blue
    uint8_t a;  // alpha
};

struct Camera
{
    glm::vec3 position;
    glm::vec3 target;
};

struct Face
{
    uint16_t a;
    uint16_t b;
    uint16_t c;
};
// Note: with uint16, a mesh cannot exceed 65535 vertices

struct Mesh
{
    glm::vec3 position;
    glm::vec3 rotation;
    std::vector<glm::vec3> vertices;
    std::vector<Face> faces;
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
              m_window, -1,
              SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC) )
    { }

    ~Device()
    {
        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
    }

    // This method is called to clear the back buffer with a specific color
    void clear(color4 c)
    {
        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderClear(m_renderer);
    }

    // Once everything is ready, we can flush the back buffer into the front buffer
    void present()
    {
        SDL_RenderPresent(m_renderer);
    }

    // DrawPoint calls PutPixel but does the clipping operation before
    void drawPoint(glm::vec2 p, color4 c)
    {
        if(p.x >= 0 && p.y >= 0 &&
           p.x < m_winWidth &&
           p.y < m_winHeight      )
        {
            putPixel(static_cast<uint16_t>(p.x), static_cast<uint16_t>(p.y), c);
        }
    }

    void drawLine(glm::vec2 p0, glm::vec2 p1)
    {
        // Bresenham's line algorithm
        // https://en.wikipedia.org/wiki/Bresenham's_line_algorithm

              auto x0 = static_cast<uint16_t>(p0.x);
              auto y0 = static_cast<uint16_t>(p0.y);
        const auto x1 = static_cast<uint16_t>(p1.x);
        const auto y1 = static_cast<uint16_t>(p1.y);

        const auto dx = std::abs(x1 - x0);
        const auto dy = std::abs(y1 - y0);
        const auto sx = (x0 < x1) ? 1 : -1;
        const auto sy = (y0 < y1) ? 1 : -1;
        auto err = dx - dy;

        while (true) {
            drawPoint(glm::vec2(x0, y0), {255, 255, 255, 255});

            if ((x0 == x1) && (y0 == y1)) break;
            const auto e2 = 2 * err;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }

    // The main method of the engine that re-compute each vertex projection during each frame
    void render(Camera camera, std::vector<Mesh> meshes)
    {
        const auto viewport = glm::vec4(0, 0, m_winWidth, m_winHeight);
        const auto viewMat = glm::lookAtLH(camera.position, camera.target, glm::vec3(0,1,0));
        const auto projMat = glm::perspectiveFovLH(
            0.78f,
            static_cast<float>(m_winWidth),
            static_cast<float>(m_winHeight),
            0.01f,
            1.0f
        );

        for(const auto &mesh : meshes)
        {
            // Beware to apply rotation before translation
            const auto transMat = glm::translate(glm::mat4(1.0f), mesh.position);
            const auto rotXMat = glm::rotate(transMat, mesh.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
            const auto rotYMat = glm::rotate(rotXMat, mesh.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
            const auto rotZMat = glm::rotate(rotYMat, mesh.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
            const auto modelMat = rotZMat;
            // Note1: the tutorial names this matrice "worldMatrice".
            // Giving up the nice Futurama quote, and naming it "modelMatrice" to follow GDM examples.

            // Note2: the tutorial merges all matrices at last
            // const auto mvpMap = projMat * viewMat * modelMat;
            // …but GLM project function expects ModelView and Projection matrices separately
            const auto mvMat = viewMat * modelMat;

            for(const auto face : mesh.faces)
            {
                const auto vertexA = mesh.vertices[face.a];
                const auto vertexB = mesh.vertices[face.b];
                const auto vertexC = mesh.vertices[face.c];

                const auto pixelA = glm::project(vertexA, mvMat, projMat, viewport);
                const auto pixelB = glm::project(vertexB, mvMat, projMat, viewport);
                const auto pixelC = glm::project(vertexC, mvMat, projMat, viewport);

                drawLine(pixelA, pixelB);
                drawLine(pixelB, pixelC);
                drawLine(pixelC, pixelA);
            }
        }
    }

private:
    // Called to put a pixel on screen at a specific X,Y coordinates
    void putPixel(uint16_t x, uint16_t y, color4 c)
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

    const Camera camera{
        { 0, 0, 10 },
        { 0, 0, 0 }
    };

    const Mesh cube{
        {0, 0, 0},  /* position */
        {0, 0, 0},  /* rotation */
        {           /* vertices */
            {-1,  1,  1},   // 0
            { 1,  1,  1},   // 1
            {-1, -1,  1},   // 2
            { 1, -1,  1},   // 3
            {-1,  1, -1},   // 4
            { 1,  1, -1},   // 5
            { 1, -1, -1},   // 6
            {-1, -1, -1}    // 7
        },
        {
            {0, 1, 2},      // 0
            {1, 2, 3},      // 1
            {1, 3, 6},      // 2
            {1, 5, 6},      // 3
            {0, 1, 4},      // 4
            {1, 4, 5},      // 5

            {2, 3, 7},      // 6
            {3, 6, 7},      // 7
            {0, 2, 7},      // 8
            {0, 4, 7},      // 9
            {4, 5, 6},      // 10
            {4, 6, 7}       // 11
        }
    };

    std::vector<Mesh> meshes = { cube };

    // Rendering loop
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

        // rotating slightly the cube during each frame rendered
        auto& cubeRot = meshes[0].rotation;
        cubeRot = glm::vec3(cubeRot.x += 0.01, cubeRot.y += 0.01, cubeRot.z += 0.01);

        // Doing the various matrix operations
        device.render(camera, meshes);

        // Flushing the back buffer into the front buffer
        device.present();
    }

    SDL_Quit();

    return 0;
}
