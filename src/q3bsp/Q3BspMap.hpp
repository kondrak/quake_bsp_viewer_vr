#ifndef Q3BSPMAP_INCLUDED
#define Q3BSPMAP_INCLUDED

#include "Frustum.hpp"
#include "common/BspMap.hpp"
#include "q3bsp/Q3Bsp.hpp"
#include "renderer/OpenGL.hpp"
#include <vector>
#include <map>

class  Texture;
struct Q3BspPatch;

/*
 *  Quake III map
 */

class Q3BspMap : public BspMap
{
public:
    static const int   s_tesselationLevel; // level of curved surface tesselation
    static const float s_worldScale;       // scale down factor for the map

    Q3BspMap() : BspMap(), m_lightmapTextures(NULL)
    {
    }

    ~Q3BspMap();

    void Init();
    void OnRenderStart();
    void Render();
    void OnRenderFinish();

    bool ClusterVisible(int cameraCluster, int testCluster)   const;
    int  FindCameraLeaf(const Math::Vector3f &cameraPosition) const;
    void CalculateVisibleFaces(const Math::Vector3f &cameraPosition);

    // bsp data
    Q3BspHeader     header;
    Q3BspEntityLump entities;
    std::vector<Q3BspTextureLump>   textures;
    std::vector<Q3BspPlaneLump>     planes;
    std::vector<Q3BspNodeLump>      nodes;
    std::vector<Q3BspLeafLump>      leaves;
    std::vector<Q3BspLeafFaceLump>  leafFaces;
    std::vector<Q3BspLeafBrushLump> leafBrushes;
    std::vector<Q3BspModelLump>     models;
    std::vector<Q3BspBrushLump>     brushes;
    std::vector<Q3BspBrushSideLump> brushSides;
    std::vector<Q3BspVertexLump>    vertices;
    std::vector<Q3BspMeshVertLump>  meshVertices;
    std::vector<Q3BspEffectLump>    effects;
    std::vector<Q3BspFaceLump>      faces;
    std::vector<Q3BspLightMapLump>  lightMaps;
    std::vector<Q3BspLightVolLump>  lightVols;
    Q3BspVisDataLump                visData;

private:
    void LoadTextures();
    void LoadLightmaps();
    void SetLightmapGamma(float gamma);
    void CreatePatch(const Q3BspFaceLump &f);
    void RenderFace(int idx);
    void RenderPatch(int idx);

    // VBO creation
    void CreateBuffersForFace(const Q3BspFaceLump &face, int idx);
    void CreateBuffersForPatch(int idx);

    // render data
    std::vector<Q3LeafRenderable>   m_renderLeaves; // bsp leaves in "renderable format"
    std::vector<Q3FaceRenderable>   m_renderFaces;  // bsp faces in "renderable format"

    std::vector<Q3BspPatch *>       m_patches;      // curved surfaces
    std::vector<Texture *>          m_textures;     // loaded in-game textures
    std::vector<Q3FaceRenderable *> m_visibleFaces; // list of visible surfaces to render
    GLuint  *m_lightmapTextures;                    // bsp lightmaps

    Frustum  m_frustum;                             // view frustum

    // helper textures
    Texture *m_missingTex;   // rendered if an in-game texture is missing
    GLuint   m_whiteTex;     // used if no lightmap specified for a face

    // rendering buffers (VAO + VBO)
    RenderBuffers m_renderBuffers;
};


#endif