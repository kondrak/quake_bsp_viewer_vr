#ifndef Q3BSPRENDERHELPERS_INCLUDED
#define Q3BSPRENDERHELPERS_INCLUDED

#include "renderer/Math.hpp"
#include "renderer/OpenGL.hpp"
#include <vector>
#include <map>

/*
 * Helper structs for Q3BspMap rendering
 */

enum Q3BspRenderFlags
{
    Q3RenderShowWireframe  = 1 << 0,
    Q3RenderShowLightmaps  = 1 << 1,
    Q3RenderUseLightmaps   = 1 << 2,
    Q3RenderAlphaTest      = 1 << 3,
    Q3RenderSkipMissingTex = 1 << 4,
    Q3RenderSkipPVS        = 1 << 5,
    Q3RenderSkipFC         = 1 << 6
};


// leaf structure used for occlusion culling (PVS/frustum)
struct Q3LeafRenderable
{
    int visCluster;
    int firstFace;
    int numFaces;
    Math::Vector3f boundingBoxVertices[8];
};


// face structure used for rendering
struct Q3FaceRenderable
{
    int type;
    int index;
};


// VBO handles for a single face in the BSP
struct FaceBuffers
{
    GLuint m_vertexBuffer;
    GLuint m_texcoordBuffer;
    GLuint m_lightmapTexcoordBuffer;
};


struct RenderBuffers
{
    GLuint m_vertexArray; // single vertex vertex array for the entire map

    std::map< int, FaceBuffers > m_faceVBOs;
    std::map< int, std::vector<FaceBuffers> > m_patchVBOs;
};


// map statistics
struct BspStats
{
    BspStats() : totalVertices(0), 
                 totalFaces(0), 
                 visibleFaces(0), 
                 totalPatches(0), 
                 visiblePatches(0)
    {
    }

    int totalVertices;
    int totalFaces;
    int visibleFaces;
    int totalPatches;
    int visiblePatches;
};



#endif