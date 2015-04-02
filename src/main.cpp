#include "Application.hpp"
#include "InputHandlers.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/OculusVR.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/CameraDirector.hpp"

// for simplicity, let's use globals
RenderContext  g_renderContext;
Application    g_application;
OculusVR       g_oculusVR;
CameraDirector g_cameraDirector;

int main(int argc, char **argv)
{
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        LOG_MESSAGE_ASSERT(false, "Failed to initialize SDL.");
        return 1;
    }

    bool vrMode     = false;
    bool vrWindowed = false;

    for (int i = 1; i < argc; ++i)
    {
        if (!strncmp(argv[i], "-vr", 3))
            vrMode = true;

        if (!strncmp(argv[i], "-window", 7))
            vrWindowed = true;
    }

    if (vrMode)
    {
        if (!g_oculusVR.InitVR())
        {
            LOG_MESSAGE_ASSERT(false, "Failed to create VR device.");
            SDL_Quit();
            return 1;
        }
        OVR::Vector4i windowDim = g_oculusVR.RenderDimensions();

        g_renderContext.Init("Quake BSP Viewer VR", vrWindowed ? 100  : windowDim.x, vrWindowed ? 100 : windowDim.y, 
                                                    vrWindowed ? 1100 : windowDim.z, vrWindowed ? 618 : windowDim.w);
    }
    else
    {
        g_renderContext.Init("Quake BSP Viewer", 100, 100, 1024, 768);
    }

    // initialize Glew
    if (glewInit() != GLEW_OK)
    {
        LOG_MESSAGE_ASSERT(false, "Failed to initialize Glew.");
        SDL_Quit();
        return 1;
    }

    ShaderManager::GetInstance()->LoadShaders();

    if (vrMode)
    {
        if (!g_oculusVR.InitVRBuffers())
        {
            LOG_MESSAGE_ASSERT(false, "Failed to create VR render buffers.");
            g_renderContext.Destroy();
            g_oculusVR.DestroyVR();
            SDL_Quit();
            return 1;
        }

        SDL_SysWMinfo info;
        memset(&info, 0, sizeof(SDL_SysWMinfo));
        SDL_VERSION(&info.version);
        SDL_GetWindowWMInfo(g_renderContext.window, &info);

        OVR::Vector4i windowDim = g_oculusVR.RenderDimensions();
        g_oculusVR.ConfigureRender(info.info.win.window, vrWindowed ? 1100 : windowDim.z, vrWindowed ? 618 : windowDim.w);
        g_oculusVR.CreateDebug();
    }
    else
    {
        // initialize the viewport
        g_application.OnWindowResize(g_renderContext.width, g_renderContext.height);
    }

    SDL_ShowCursor( SDL_DISABLE );
    g_application.OnStart( argc, argv, vrMode );

    double now = 0, last = 0; 

    while (g_application.Running())
    {
        // handle key presses
        processEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        if (g_application.VREnabled())
        {
            now = ovr_GetTimeInSeconds();
            float dt = float(now - last);

            g_oculusVR.OnRenderStart();
            g_application.OnUpdate(dt);

            Math::Vector3f camPos = g_cameraDirector.GetActiveCamera()->Position();

            for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
            {               
                OVR::Matrix4f OVRMVP = g_oculusVR.OnEyeRender(eyeIndex);
                OVR::Matrix4f MVPMatrix = (OVRMVP * OVR::Matrix4f(OVR::Quatf(OVR::Vector3f(1.0, 0.0, 0.0), -PIdiv2)) // rotate 90 degrees by X axis (BSP world is flipped)
                                                  * OVR::Matrix4f::Translation(-OVR::Vector3f(camPos.m_x, camPos.m_y, camPos.m_z))).Transposed();

                // override camera right, up and view vectors by the ones stored in OVR MVP matrix
                g_cameraDirector.GetActiveCamera()->SetRightVector(MVPMatrix.M[0][0], MVPMatrix.M[1][0], MVPMatrix.M[2][0]);
                g_cameraDirector.GetActiveCamera()->SetUpVector(   MVPMatrix.M[0][1], MVPMatrix.M[1][1], MVPMatrix.M[2][1]);
                g_cameraDirector.GetActiveCamera()->SetViewVector( MVPMatrix.M[0][2], MVPMatrix.M[1][2], MVPMatrix.M[2][2]);
                
                g_renderContext.ModelViewProjectionMatrix = Math::Matrix4f(&MVPMatrix.M[0][0]);

                // camera frustum should use the non-inverted and non-translated MVP (fixed position, correct orientation)
                glUniformMatrix4fv(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::OVRFrustumShader).uniforms[ModelViewProjectionMatrix], 1, GL_FALSE, &OVRMVP.Transposed().M[0][0]);
                g_application.OnRender();  
            }

            g_oculusVR.OnRenderEnd();
        }
        else
        {
            now = SDL_GetTicks();
            float dt = float(now - last) / 1000.f;

            g_application.OnUpdate(dt);

            g_renderContext.ModelViewProjectionMatrix = g_cameraDirector.GetActiveCamera()->ViewMatrix() * g_cameraDirector.GetActiveCamera()->ProjectionMatrix();
            g_application.OnRender();

            SDL_GL_SwapWindow(g_renderContext.window);
        }

        last = now;
    }

    g_application.OnTerminate();
    g_renderContext.Destroy();
    g_oculusVR.DestroyVR();
    SDL_Quit();

    return 0;
}