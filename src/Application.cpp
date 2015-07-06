#include <SDL.h>
#include "Application.hpp"
#include "StringHelpers.hpp"
#include "renderer/CameraDirector.hpp"
#include "renderer/RenderContext.hpp"
#include "renderer/ShaderManager.hpp"
#include "q3bsp/Q3BspLoader.hpp"
#include "q3bsp/Q3BspStatsUI.hpp"
#include "renderer/OculusVR.hpp"

extern RenderContext  g_renderContext;
extern OculusVR       g_oculusVR;
extern CameraDirector g_cameraDirector;


void Application::OnWindowResize(int newWidth, int newHeight)
{
    g_renderContext.width  = newWidth;
    g_renderContext.height = newHeight;

    glViewport(0, 0, newWidth, newHeight);
}

void Application::OnStart(int argc, char **argv, bool vrMode)
{
    m_VREnabled = vrMode;   
    glEnable(GL_MULTISAMPLE);

    Q3BspLoader loader;
    // assume the parameter with a string ".bsp" is the map we want to load
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]).find(".bsp") != std::string::npos)
        {
            m_q3map = loader.Load(argv[i]);
            break;
        }
    }

    Math::Vector3f startPos;

    if (m_q3map)
    {
        m_q3map->Init();
        m_q3map->ToggleRenderFlag(Q3RenderUseLightmaps);

        // try to locate the first info_player_deathmatch entity and place the camera there
        startPos = FindPlayerStart(static_cast<Q3BspMap *>(m_q3map)->entities.ents);
    }

    g_cameraDirector.AddCamera(startPos / Q3BspMap::s_worldScale,
                               Math::Vector3f(0.f, 0.f, 1.f),
                               Math::Vector3f(1.f, 0.f, 0.f),
                               Math::Vector3f(0.f, 1.f, 0.f));

    // set to "clean" perspective matrix
    g_cameraDirector.GetActiveCamera()->SetMode(Camera::CAM_FPS);

    m_q3stats = new Q3StatsUI(m_q3map);
}


void Application::OnRender()
{
    //update global MVP matrix in primary shader    
    glUniformMatrix4fv(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader).uniforms[ModelViewProjectionMatrix], 1, GL_FALSE, &(g_renderContext.ModelViewProjectionMatrix[0]));

    // render the bsp
    if (m_q3map)
    {
        // no need to calculate separate view matrix if we're in VR (OVR handles it for us)
        if (!VREnabled())
        {
            g_cameraDirector.GetActiveCamera()->OnRender();
        }

        m_q3map->OnRenderStart();
        m_q3map->Render();
        m_q3map->OnRenderFinish();
    }

    // render map stats
    switch (m_debugRenderState)
    {
    case RenderMapStats:
        m_q3stats->Render();
        break;
    case RenderVRData:
        g_oculusVR.RenderDebug();
        break;
    case RenderVRTrackingCamera:
        g_oculusVR.RenderTrackerFrustum();
    default:
        break;
    }
}


void Application::OnUpdate(float dt)
{
    UpdateCamera(dt);

    // determine which faces are visible
    if (m_q3map)
        m_q3map->CalculateVisibleFaces(g_cameraDirector.GetActiveCamera()->Position());

    if (VREnabled())
    {
        g_oculusVR.UpdateDebug();
    }
}


void Application::OnTerminate()
{
    delete m_q3map;
    delete m_q3stats;
}


bool Application::KeyPressed(KeyCode key)
{
    // to be 100% no undefined state exists
    if (m_keyStates.find(key) == m_keyStates.end())
        m_keyStates[key] = false;

    return m_keyStates[key];
}


void Application::OnKeyPress(KeyCode key)
{
    SetKeyPressed(key, true);

    switch (key)
    {
    case KEY_F1:
        m_q3map->ToggleRenderFlag(Q3RenderShowWireframe);
        break;
    case KEY_F2:
        m_q3map->ToggleRenderFlag(Q3RenderShowLightmaps);
        glUniform1i(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader).uniforms[RenderLightmaps], m_q3map->HasRenderFlag(Q3RenderShowLightmaps) ? 1 : 0);
        break;
    case KEY_F3:
        m_q3map->ToggleRenderFlag(Q3RenderUseLightmaps);
        glUniform1i(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader).uniforms[UseLightmaps], m_q3map->HasRenderFlag(Q3RenderUseLightmaps) ? 1 : 0);
        break;
    case KEY_F4:
        m_q3map->ToggleRenderFlag(Q3RenderAlphaTest);
        glUniform1i(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader).uniforms[UseAlphaTest], m_q3map->HasRenderFlag(Q3RenderAlphaTest) ? 1 : 0);
        break;
    case KEY_F5:
        m_q3map->ToggleRenderFlag(Q3RenderSkipMissingTex);
        break;
    case KEY_F6:
        m_q3map->ToggleRenderFlag(Q3RenderSkipPVS);
        break;
    case KEY_F7:
        m_q3map->ToggleRenderFlag(Q3RenderSkipFC);
        break;
    case KEY_TILDE:
        m_debugRenderState++;
        if (!VREnabled())
        {
            if (m_debugRenderState > RenderMapStats)
                m_debugRenderState = None;
        }
        else
        {
            if (g_oculusVR.IsDebugHMD() || !g_oculusVR.IsDK2())
            {
                if (m_debugRenderState == RenderVRTrackingCamera)
                    m_debugRenderState++;
            }

            // attempt to render performance hud with debug device causes a crash (SDK 0.6.0.1)
            if (g_oculusVR.IsDebugHMD() && m_debugRenderState > RenderVRTrackingCamera)
            {
                m_debugRenderState = None;
            }

            switch (m_debugRenderState)
            {
            case RenderDK2LatencyTiming:
                g_oculusVR.ShowPerfStats(ovrPerfHud_LatencyTiming);
                break;
            case RenderDK2RenderTiming:
                g_oculusVR.ShowPerfStats(ovrPerfHud_RenderTiming);
                break;
            default:
                g_oculusVR.ShowPerfStats(ovrPerfHud_Off);
                break;
            }
        }
        if (m_debugRenderState >= DebugRenderMax)
            m_debugRenderState = None;
        break;
    case KEY_M:
        m_mirrorMode++;

        if (m_mirrorMode == MM_Count)
            m_mirrorMode = Mirror_Regular;
        break;
    case KEY_ESC:
        Terminate();
        break;
    default:
        break;
    }

    if (VREnabled())
    {
        g_oculusVR.OnKeyPress(key);
    }
}


void Application::OnKeyRelease(KeyCode key)
{
    SetKeyPressed(key, false);
}


void Application::OnMouseMove(int x, int y)
{
    if (!VREnabled())
    {
        g_cameraDirector.GetActiveCamera()->OnMouseMove(x, y);
    }
}


void Application::UpdateCamera(float dt)
{
    // don't make movement too sickening in VR
    static const float movementSpeed = VREnabled() ? 1.f : 8.f;

    if (KeyPressed(KEY_A))
        g_cameraDirector.GetActiveCamera()->Strafe(-movementSpeed * dt);

    if (KeyPressed(KEY_D))
        g_cameraDirector.GetActiveCamera()->Strafe(movementSpeed * dt);

    if (KeyPressed(KEY_W))
        g_cameraDirector.GetActiveCamera()->MoveForward(-movementSpeed * dt);

    if (KeyPressed(KEY_S))
        g_cameraDirector.GetActiveCamera()->MoveForward(movementSpeed * dt);

    // do the barrel roll!
    if (KeyPressed(KEY_Q))
        g_cameraDirector.GetActiveCamera()->rotateZ(2.f * dt);

    if (KeyPressed(KEY_E))
        g_cameraDirector.GetActiveCamera()->rotateZ(-2.f * dt);

    // move straight up/down
    if (KeyPressed(KEY_R))
        g_cameraDirector.GetActiveCamera()->MoveUpward(movementSpeed * dt);

    if (KeyPressed(KEY_F))
        g_cameraDirector.GetActiveCamera()->MoveUpward(-movementSpeed * dt);
}


Math::Vector3f Application::FindPlayerStart(const char *entities)
{
    std::string str(entities);
    Math::Vector3f result(0.0f, 0.0f, 4.0f); // some arbitrary position in case there's no info_player_deathmatch on map
    size_t pos = 0;

    std::string playerStartCoords("");

    while (pos != std::string::npos)
    {
        size_t posOpen = str.find("{", pos);
        size_t posClose = str.find("}", posOpen);

        if (posOpen != std::string::npos)
        {
            std::string entity = str.substr(posOpen + 1, posClose - posOpen - 1);
            if (FindEntityAttribute(entity, "info_player_deathmatch", "origin", playerStartCoords))
                break;
        }

        pos = posClose;
    }

    if (!playerStartCoords.empty())
    {
        auto coords = StringHelpers::tokenizeString(playerStartCoords.c_str(), ' ');

        result.m_x = std::stof(coords[0]);
        result.m_y = std::stof(coords[1]);
        result.m_z = std::stof(coords[2]);
    }

    return result;
}


bool Application::FindEntityAttribute(const std::string &entity, const char *entityName, const char *attribName, std::string &output)
{
    output.clear();
    std::string trimmedEntity = StringHelpers::trim(entity, '\n');
    std::string attribValueFinal("");
    std::string attribValueRead("");
    bool attribFound = false;

    auto tokens = StringHelpers::tokenizeString(trimmedEntity.c_str(), '\n');

    for (auto &t : tokens)
    {
        auto attribs = StringHelpers::tokenizeString(t.c_str(), ' ', 2);

        for (auto &a : attribs)
        {
            a = StringHelpers::trim(a, '"');
        }

        if (!strncmp(attribs[0].c_str(), attribName, strlen(attribs[0].c_str())))
        {
            attribValueRead = attribs[1];
        }

        if (!strncmp(attribs[1].c_str(), entityName, strlen(attribs[1].c_str())))
        {   
            attribFound = true;
        }
    }

    if (attribFound)
        output = attribValueRead;

    return attribFound;
}