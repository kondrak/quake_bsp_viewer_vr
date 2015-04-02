#ifndef APPLICATION_INCLUDED
#define APPLICATION_INCLUDED

#include <map>
#include "InputHandlers.hpp"
#include "renderer/Math.hpp"

class BspMap;
class StatsUI;

/*
 * main application 
 */

class Application
{
public:
    Application() : m_running(true), m_VREnabled(false), m_q3map(NULL), m_q3stats(NULL), m_debugRenderState(RenderMapStats)
    {
    }

    void OnWindowResize(int newWidth, int newHeight);

    void OnStart(int argc, char **argv, bool vrMode);
    void OnRender();
    void OnUpdate(float dt); 

    inline bool Running() const  { return m_running; }
    inline void Terminate()      { m_running = false; }

    void OnTerminate();
    bool KeyPressed(KeyCode key);
    void OnKeyPress(KeyCode key);
    void OnKeyRelease(KeyCode key);
    void OnMouseMove(int x, int y);
    bool VREnabled() const { return m_VREnabled; }
private:
    void UpdateCamera( float dt );
    inline void SetKeyPressed(KeyCode key, bool pressed) { m_keyStates[key] = pressed; }

    // helper functions for parsing Quake entities
    Math::Vector3f FindPlayerStart(const char *entities);
    bool FindEntityAttribute(const std::string &entity, const char *entityName, const char *attribName, std::string &output);


    bool m_running;
    bool m_VREnabled;
    std::map< KeyCode, bool > m_keyStates; 

    BspMap  *m_q3map   = NULL;    // loaded map
    StatsUI *m_q3stats = NULL;    // map stats UI

    enum DebugRender
    {
        None = 0,
        RenderMapStats,
        RenderVRData,
        RenderVRTrackingCamera,
        DebugRenderMax
    };

    unsigned int m_debugRenderState;
};

#endif
