#ifndef BSPMAP_INCLUDED
#define BSPMAP_INCLUDED

#include "q3bsp/Q3BspRenderHelpers.hpp"

/*
 *  Base class for renderable bsp map. Future reference for other BSP format support.
 */

class BspMap
{
public:
    BspMap() : m_renderFlags(0)
    {
    }

    virtual ~BspMap()
    {
    }

    virtual void Init() = 0;
    virtual void OnRenderStart()  = 0;  // prepare for render
    virtual void Render()         = 0;  // perform rendering
    virtual void OnRenderFinish() = 0;  // finish render

    virtual bool ClusterVisible(int cameraCluster, int testCluster) const    = 0;  // determine bsp cluster visibility
    virtual int  FindCameraLeaf(const Math::Vector3f &cameraPosition) const  = 0;  // return bsp leaf index containing the camera
    virtual void CalculateVisibleFaces(const Math::Vector3f &cameraPosition) = 0;  // determine which bsp faces are visible

    // render helpers - extra flags + map statistics
    inline void  ToggleRenderFlag(int flag)    { m_renderFlags ^= flag; }
    inline bool  HasRenderFlag(int flag) const { return (m_renderFlags & flag) == flag; }
    inline const BspStats &GetMapStats() const { return m_mapStats; }

protected:
    int      m_renderFlags;
    BspStats m_mapStats;
};


#endif