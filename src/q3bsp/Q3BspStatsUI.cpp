#include "Application.hpp"
#include "q3bsp/Q3BspMap.hpp"
#include "q3bsp/Q3BspStatsUI.hpp"
#include "renderer/RenderContext.hpp"
#include <sstream>

extern RenderContext g_renderContext;
extern Application   g_application;

Q3StatsUI::Q3StatsUI(BspMap *map) : StatsUI(map)
{
    m_font = new Font("res/font.png");

    // make the font slightly bigger in VR
    if (g_application.VREnabled())
        m_font->SetScale(Math::Vector2f(3.f * (float)g_renderContext.height / 1080.f, 3.f * (float)g_renderContext.height / 1080.f));
    else
        m_font->SetScale(Math::Vector2f(2.f, 2.f));
}


void Q3StatsUI::Render()
{
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glActiveTexture(GL_TEXTURE0);

    // no map loaded or no cmdline parameter specified - display error message
    if(!m_map)
    {
        m_font->SetColor(Math::Vector4f(1.f, 0.f, 0.f, 1.f));
        m_font->drawText("Error loading BSP - missing/corrupted file or no file specified!", -0.7f, 0.0f, 0.f);
        return;
    }

    // check if we should rescale the font for non-VR mode here, since user can change window size dynamically (unlike VR mode)
    if (!g_application.VREnabled())
        m_font->SetScale(Math::Vector2f(2.f * (float)g_renderContext.height / 768.f, 2.f * (float)g_renderContext.height / 768.f));

    static const float statsX   = g_application.VREnabled() ? -0.19f : -0.99f;
    static const float keysX    = g_application.VREnabled() ? -0.19f :  0.35f;
    static const float statsY   = g_application.VREnabled() ?  0.25f :  0.70f;
    static const float keysY    = g_application.VREnabled() ? -0.02f : -0.25f;
    static const float ySpacing = 0.05f;

    const BspStats &stats = m_map->GetMapStats();

    std::stringstream statsStream;

    statsStream << "Total vertices: " << stats.totalVertices;
    m_font->drawText(statsStream.str(), statsX, statsY, 0.f);

    statsStream.str("");
    statsStream << "Total faces: " << stats.totalFaces;
    m_font->drawText(statsStream.str(), statsX, statsY - ySpacing, 0.f);

    statsStream.str("");
    statsStream << "Total patches: " << stats.totalPatches;
    m_font->drawText(statsStream.str(), statsX, statsY - ySpacing * 2.f, 0.f);

    statsStream.str("");
    statsStream << "Rendered faces: " << stats.visibleFaces;
    m_font->drawText(statsStream.str(), statsX, statsY - ySpacing * 3.f, 0.f);

    statsStream.str("");
    statsStream << "Rendered patches: " << stats.visiblePatches;
    m_font->drawText(statsStream.str(), statsX, statsY - ySpacing * 4.f, 0.);

    m_font->SetColor(Math::Vector4f(1.f, 0.f, 0.f, 1.f));
    m_font->drawText(" ~ - toggle stats view", keysX, keysY, 0.f);

    if (m_map->HasRenderFlag(Q3RenderShowWireframe))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    else
        m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));
    m_font->drawText("F1 - show wireframe", keysX, keysY - ySpacing, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderShowLightmaps))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F2 - show lightmaps", keysX, keysY - ySpacing * 2.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderUseLightmaps))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F3 - use lightmaps", keysX, keysY - ySpacing * 3.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (m_map->HasRenderFlag(Q3RenderAlphaTest))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F4 - use alpha test", keysX, keysY - ySpacing * 4.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipMissingTex))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F5 - show missing textures", keysX, keysY - ySpacing * 5.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipPVS))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F6 - use PVS culling", keysX, keysY - ySpacing * 6.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));

    if (!m_map->HasRenderFlag(Q3RenderSkipFC))
        m_font->SetColor(Math::Vector4f(0.f, 1.f, 0.f, 1.f));
    m_font->drawText("F7 - use frustum culling", keysX, keysY - ySpacing * 7.f, 0.f);
    m_font->SetColor(Math::Vector4f(1.f, 1.f, 1.f, 1.f));
}
