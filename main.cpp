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
#include <limits>   // std::numeric_limits
#include <cmath>    // std::abs, std::lerp
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


namespace std {
    // std::lerp is only available from c++17
    constexpr float clamp(float v, float lo, float hi)
    {
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }

    // std::lerp is only available from c++20
    constexpr float lerp(float a, float b, float t) {
        return a + clamp(t, 0, 1)*(b-a);
    }
}

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
        , m_depthBuffer(m_winWidth * m_winHeight, std::numeric_limits<float>::max())
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

        // clear depth buffer (aka z-buffer)
        for(uint32_t i = 0; i < m_depthBuffer.size(); i++)
        {
            m_depthBuffer[i] = std::numeric_limits<float>::max();
        }
    }

    // Once everything is ready, we can flush the back buffer into the front buffer
    void present()
    {
        SDL_RenderPresent(m_renderer);
    }

    // DrawPoint calls PutPixel but does the clipping operation before
    void drawPoint(glm::vec3 p, color4 c)
    {
        if(p.x >= 0 && p.y >= 0 &&
           p.x < m_winWidth &&
           p.y < m_winHeight      )
        {
            putPixel(static_cast<uint16_t>(p.x),
                     static_cast<uint16_t>(p.y),
                     p.z,
                     c);
        }
    }

    /*
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
    */

    void processScanline(uint16_t y,
                         glm::vec3 pa, glm::vec3 pb, glm::vec3 pc, glm::vec3 pd,
                         color4 c)
    {
        // Thanks to current Y, we can compute the gradient to compute others values like
        // the starting X (sx) and ending X (ex) to draw between
        // if pa.Y == pb.Y or pc.Y == pd.Y, gradient is forced to 1
        const auto gradient1 = (pa.y != pb.y) ? (y - pa.y) / (pb.y - pa.y) : 1;
        const auto gradient2 = (pc.y != pd.y) ? (y - pc.y) / (pd.y - pc.y) : 1;

        // Note: std::lerp for "interpolate"
        // See: https://en.cppreference.com/w/cpp/numeric/lerp
        const uint16_t sx = static_cast<uint16_t>(std::lerp(pa.x, pb.x, gradient1));
        const uint16_t ex = static_cast<uint16_t>(std::lerp(pc.x, pd.x, gradient2));

        // starting Z & ending Z
        const float z1 = std::lerp(pa.z, pb.z, gradient1);
        const float z2 = std::lerp(pc.z, pd.z, gradient2);

        // drawing a line from left (sx) to right (ex)
        for(auto x = sx; x < ex; x++)
        {
            const float gradient = (x - sx) / static_cast<float>(ex - sx);
            const float z = std::lerp(z1, z2, gradient);

            drawPoint(glm::vec3(x, y, z), c);
        }
    }

    void drawTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, color4 c)
    {
        // Sorting the points in order to always have this order on screen p1, p2 & p3
        // with p1 always up (thus having the Y the lowest possible to be near the top screen)
        // then p2 between p1 & p3
        if(p1.y > p2.y)
        {
            const auto temp = p2;
            p2 = p1;
            p1 = temp;
        }

        if(p2.y > p3.y)
        {
            const auto temp = p2;
            p2 = p3;
            p3 = temp;
        }

        if(p1.y > p2.y)
        {
            const auto temp = p2;
            p2 = p1;
            p1 = temp;
        }

        // inverse slopes
        float dP1P2, dP1P3;

        // http://en.wikipedia.org/wiki/Slope
        // Computing inverse slopes
        if(p2.y - p1.y > 0)
            dP1P2 = (p2.x - p1.x) / (p2.y - p1.y);
        else
            dP1P2 = 0;

        if (p3.y - p1.y > 0)
            dP1P3 = (p3.x - p1.x) / (p3.y - p1.y);
        else
            dP1P3 = 0;

        // First case where triangles are like that:
        // P1
        // -
        // --
        // - -
        // -  -
        // -   - P2
        // -  -
        // - -
        // -
        // P3
        if(dP1P2 > dP1P3)
        {
            for(auto y = static_cast<uint16_t>(p1.y); y <= static_cast<uint16_t>(p3.y); y++)
            {
                if(y < p2.y)
                {
                    processScanline(y, p1, p3, p1, p2, c);
                }
                else
                {
                    processScanline(y, p1, p3, p2, p3, c);
                }
            }
        }
        // Second case where triangles are like that:
        //       P1
        //        -
        //       --
        //      - -
        //     -  -
        // P2 -   -
        //     -  -
        //      - -
        //        -
        //       P3
        else
        {
            for(auto y = static_cast<uint16_t>(p1.y); y <= static_cast<uint16_t>(p3.y); y++)
            {
                if(y < p2.y)
                {
                    processScanline(y, p1, p2, p1, p3, c);
                }
                else
                {
                    processScanline(y, p2, p3, p1, p3, c);
                }
            }
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

            uint16_t faceIdx = 0;
            for(const auto face : mesh.faces)
            {
                const auto vertexA = mesh.vertices[face.a];
                const auto vertexB = mesh.vertices[face.b];
                const auto vertexC = mesh.vertices[face.c];

                const auto pixelA = glm::project(vertexA, mvMat, projMat, viewport);
                const auto pixelB = glm::project(vertexB, mvMat, projMat, viewport);
                const auto pixelC = glm::project(vertexC, mvMat, projMat, viewport);

                const bool alt = (faceIdx % 2 == 0);
                drawTriangle(pixelA, pixelB, pixelC, {alt ? 255 : 0, 0, alt ? 0 : 255, 255});
                faceIdx++;
            }
        }
    }

private:
    // Called to put a pixel on screen at a specific X,Y coordinates
    void putPixel(uint16_t x, uint16_t y, float z, color4 c)
    {
        const auto idx = x + y*m_winWidth;

        if(m_depthBuffer[idx] < z)
        {
            return; // Discard
        }
        m_depthBuffer[idx] = z;

        SDL_SetRenderDrawColor(m_renderer, c.r, c.g, c.b, c.a);
        SDL_RenderDrawPoint(m_renderer, x, y);
    }

private:
    const uint16_t m_winWidth;
    const uint16_t m_winHeight;

private:
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;

private:
    std::vector<float> m_depthBuffer;
    // Note: this needs to be the same type as inside glm::vec3
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
