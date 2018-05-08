#include "Application.hpp"
#include "InputHandlers.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/CameraDirector.hpp"

// for simplicity, let's use globals
RenderContext  g_renderContext;
Application    g_application;
CameraDirector g_cameraDirector;

int main(int argc, char **argv)
{
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        LOG_MESSAGE_ASSERT(false, "Failed to initialize SDL.");
        return 1;
    }

    g_renderContext.Init("Quake BSP Viewer", 100, 100, 1024, 768);


    // initialize Glew
    if (glewInit() != GLEW_OK)
    {
        GLenum err = glGetError();
        LOG_MESSAGE_ASSERT(false, "Failed to initialize Glew.");
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        SDL_Quit();
        return 1;
    }

    ShaderManager::GetInstance()->LoadShaders();

    // initialize the viewport
    g_application.OnWindowResize(g_renderContext.width, g_renderContext.height);


    SDL_ShowCursor(SDL_DISABLE);
    g_application.OnStart(argc, argv);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    double now = 0, last = 0;

    while (g_application.Running())
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // handle key presses
        processEvents();

        now = SDL_GetTicks();
        float dt = float(now - last) / 1000.f;

        g_application.OnUpdate(dt);

        g_renderContext.ModelViewProjectionMatrix = g_cameraDirector.GetActiveCamera()->ViewMatrix() * g_cameraDirector.GetActiveCamera()->ProjectionMatrix();
        g_application.OnRender();

        SDL_GL_SwapWindow(g_renderContext.window);
        last = now;
    }

    g_application.OnTerminate();
    g_renderContext.Destroy();
    SDL_Quit();

    return 0;
}
