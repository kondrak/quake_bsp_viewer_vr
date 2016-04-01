#ifndef APPLICATION_INCLUDED
#define APPLICATION_INCLUDED

#include <map>
#include "InputHandlers.hpp"
#include "Math.hpp"

class BspMap;
class StatsUI;

/*
 * main application 
 */

class Application
{
public:
    // Oculus VR mirror modes
    enum VRMirrorMode
    {
        Mirror_Regular,
        Mirror_RegularLeftEye,
        Mirror_RegularRightEye,
        Mirror_NonDistort,
        Mirror_NonDistortLeftEye,
        Mirror_NonDistortRightEye,
        MM_Count
    };

    Application() : m_running(true), m_mirrorMode(Mirror_Regular), m_VREnabled(false), m_q3map(NULL), m_q3stats(NULL), m_debugRenderState(RenderMapStats)
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
    const VRMirrorMode CurrMirrorMode() const { return (VRMirrorMode)m_mirrorMode; }
private:
    void UpdateCamera( float dt );
    inline void SetKeyPressed(KeyCode key, bool pressed) { m_keyStates[key] = pressed; }

    // helper functions for parsing Quake entities
    Math::Vector3f FindPlayerStart(const char *entities);
    bool FindEntityAttribute(const std::string &entity, const char *entityName, const char *attribName, std::string &output);


    bool m_running;
    bool m_VREnabled;
    int  m_mirrorMode;

    std::map< KeyCode, bool > m_keyStates; 

    BspMap  *m_q3map;    // loaded map
    StatsUI *m_q3stats;  // map stats UI

    enum DebugRender
    {
        None = 0,
        RenderMapStats,
        RenderVRData,
        RenderVRTrackingCamera,
        RenderOVRLatencyTiming,      // added in SDK 0.6.0.1
        RenderOVRAppRenderTiming,    // added in SDK 0.6.0.1
        RenderOVRCompRenderTiming,   // added in SDK 1.3.0
        RenderOVRPerf,
        RenderOVRVersion,
        DebugRenderMax
    };

    unsigned int m_debugRenderState;
};

#endif
