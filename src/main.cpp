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

void BlitOVRMirror(ovrSizei windowSize, const Math::Vector3f &camPos);

int main(int argc, char **argv)
{
    // initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0)
    {
        LOG_MESSAGE_ASSERT(false, "Failed to initialize SDL.");
        return 1;
    }

    bool vrMode = false;

    for (int i = 1; i < argc; ++i)
    {
        if (!strncmp(argv[i], "-vr", 3))
            vrMode = true;
    }

    ovrSizei windowSize;

    if (vrMode)
    {
        if (!g_oculusVR.InitVR())
        {
            LOG_MESSAGE_ASSERT(false, "Failed to create VR device.");
            SDL_Quit();
            return 1;
        }

        ovrSizei hmdResolution = g_oculusVR.GetResolution();
        windowSize = { hmdResolution.w / 2, hmdResolution.h / 2 };

        g_renderContext.Init("Quake BSP Viewer VR", 100, 100, windowSize.w, windowSize.h);
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
        if (!g_oculusVR.InitVRBuffers(windowSize.w, windowSize.h))
        {
            LOG_MESSAGE_ASSERT(false, "Failed to create VR render buffers.");
            g_oculusVR.DestroyVR();
            g_renderContext.Destroy();
            SDL_Quit();
            return 1;
        }

        if (!g_oculusVR.InitNonDistortMirror(windowSize.w, windowSize.h))
        {
            LOG_MESSAGE_ASSERT(false, "Failed to create VR  mirror render buffers.");
            g_oculusVR.DestroyVR();
            g_renderContext.Destroy();
            SDL_Quit();
            return 1;
        }

        g_oculusVR.CreateDebug();
    }
    else
    {
        // initialize the viewport
        g_application.OnWindowResize(g_renderContext.width, g_renderContext.height);
    }

    SDL_ShowCursor(SDL_DISABLE);
    g_application.OnStart(argc, argv, vrMode);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    double now = 0, last = 0;

    while (g_application.Running())
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // handle key presses
        processEvents();

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
                g_oculusVR.OnEyeRenderFinish(eyeIndex);
            }

            g_oculusVR.SubmitFrame();

            BlitOVRMirror(windowSize, camPos);
        }
        else
        {
            now = SDL_GetTicks();
            float dt = float(now - last) / 1000.f;

            g_application.OnUpdate(dt);

            g_renderContext.ModelViewProjectionMatrix = g_cameraDirector.GetActiveCamera()->ViewMatrix() * g_cameraDirector.GetActiveCamera()->ProjectionMatrix();
            g_application.OnRender();
        }

        SDL_GL_SwapWindow(g_renderContext.window);
        last = now;
    }

    g_application.OnTerminate();
    g_oculusVR.DestroyVR();
    g_renderContext.Destroy();
    SDL_Quit();

    return 0;
}


// helper function for various Oculus Rift mirror render modes
void BlitOVRMirror(ovrSizei windowSize, const Math::Vector3f &camPos)
{
    Application::VRMirrorMode mirrorMode = g_application.CurrMirrorMode();

    // Standard mirror blit (both eyes, distorted)
    if (mirrorMode == Application::Mirror_Regular)
    {
        ClearWindow(0.f, 0.f, 0.f);
        g_oculusVR.BlitMirror(ovrEye_Count, 0);
    }

    // Standard mirror blit (left eye, distorted)
    if (mirrorMode == Application::Mirror_RegularLeftEye)
    {
        ClearWindow(0.f, 0.f, 0.f);
        g_oculusVR.BlitMirror(ovrEye_Left, windowSize.w / 4);

        // rectangle indicating we're rendering left eye
        ShaderManager::GetInstance()->DisableShader();
        glViewport(0, 0, windowSize.w, windowSize.h);
        DrawRectangle(-0.75f, 0.f, 0.1f, 0.1f, 0.f, 1.f, 0.f);
    }

    // Standard mirror blit (right eye, distorted)
    if (mirrorMode == Application::Mirror_RegularRightEye)
    {
        ClearWindow(0.f, 0.f, 0.f);
        g_oculusVR.BlitMirror(ovrEye_Right, windowSize.w / 4);

        // rectangle indicating we're rendering right eye
        ShaderManager::GetInstance()->DisableShader();
        glViewport(0, 0, windowSize.w, windowSize.h);
        DrawRectangle(0.75f, 0.f, 0.1f, 0.1f, 0.f, 1.f, 0.f);
    }

    // Both eye mirror - no distortion (requires 2 extra renders!)
    if (mirrorMode == Application::Mirror_NonDistort)
    {
        g_oculusVR.OnNonDistortMirrorStart();
        for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
        {
            const OVR::Matrix4f OVRMVP = g_oculusVR.GetEyeMVPMatrix(eyeIndex);
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

            g_oculusVR.BlitNonDistortMirror(eyeIndex == 0 ? 0 : windowSize.w / 2);

        }
    }

    // Left eye - no distortion (1 extra render)
    if (mirrorMode == Application::Mirror_NonDistortLeftEye)
    {
        ClearWindow(0.f, 0.f, 0.f);
        g_oculusVR.OnNonDistortMirrorStart();

        const OVR::Matrix4f OVRMVP = g_oculusVR.GetEyeMVPMatrix(ovrEye_Left);
        OVR::Matrix4f MVPMatrix = (OVRMVP * OVR::Matrix4f(OVR::Quatf(OVR::Vector3f(1.0, 0.0, 0.0), -PIdiv2)) // rotate 90 degrees by X axis (BSP world is flipped)
                                          * OVR::Matrix4f::Translation(-OVR::Vector3f(camPos.m_x, camPos.m_y, camPos.m_z))).Transposed();

        // override camera right, up and view vectors by the ones stored in OVR MVP matrix
        g_cameraDirector.GetActiveCamera()->SetRightVector(MVPMatrix.M[0][0], MVPMatrix.M[1][0], MVPMatrix.M[2][0]);
        g_cameraDirector.GetActiveCamera()->SetUpVector(   MVPMatrix.M[0][1], MVPMatrix.M[1][1], MVPMatrix.M[2][1]);
        g_cameraDirector.GetActiveCamera()->SetViewVector( MVPMatrix.M[0][2], MVPMatrix.M[1][2], MVPMatrix.M[2][2]);

        g_renderContext.ModelViewProjectionMatrix = Math::Matrix4f(&MVPMatrix.M[0][0]);

        g_application.OnRender();

        g_oculusVR.BlitNonDistortMirror(windowSize.w / 4);

        ShaderManager::GetInstance()->DisableShader();
        glViewport(0, 0, windowSize.w, windowSize.h);
        DrawRectangle(-0.75f, 0.f, 0.1f, 0.1f, 0.f, 1.f, 0.f);
    }

    //  Right eye - no distortion (1 extra render)
    if (mirrorMode == Application::Mirror_NonDistortRightEye)
    {
        ClearWindow(0.f, 0.f, 0.f);
        g_oculusVR.OnNonDistortMirrorStart();

        const OVR::Matrix4f OVRMVP = g_oculusVR.GetEyeMVPMatrix(ovrEye_Right);
        OVR::Matrix4f MVPMatrix = (OVRMVP * OVR::Matrix4f(OVR::Quatf(OVR::Vector3f(1.0, 0.0, 0.0), -PIdiv2)) // rotate 90 degrees by X axis (BSP world is flipped)
                                          * OVR::Matrix4f::Translation(-OVR::Vector3f(camPos.m_x, camPos.m_y, camPos.m_z))).Transposed();

        // override camera right, up and view vectors by the ones stored in OVR MVP matrix
        g_cameraDirector.GetActiveCamera()->SetRightVector(MVPMatrix.M[0][0], MVPMatrix.M[1][0], MVPMatrix.M[2][0]);
        g_cameraDirector.GetActiveCamera()->SetUpVector(   MVPMatrix.M[0][1], MVPMatrix.M[1][1], MVPMatrix.M[2][1]);
        g_cameraDirector.GetActiveCamera()->SetViewVector( MVPMatrix.M[0][2], MVPMatrix.M[1][2], MVPMatrix.M[2][2]);

        g_renderContext.ModelViewProjectionMatrix = Math::Matrix4f(&MVPMatrix.M[0][0]);

        g_application.OnRender();

        g_oculusVR.BlitNonDistortMirror(windowSize.w / 4);

        ShaderManager::GetInstance()->DisableShader();
        glViewport(0, 0, windowSize.w, windowSize.h);
        DrawRectangle(0.75f, 0.f, 0.1f, 0.1f, 0.f, 1.f, 0.f);
    }
}