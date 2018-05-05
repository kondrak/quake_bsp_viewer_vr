#include "q3bsp/Q3BspMap.hpp"
#include "q3bsp/Q3BspPatch.hpp"
#include "renderer/ShaderManager.hpp"
#include "renderer/Texture.hpp"
#include "renderer/TextureManager.hpp"
#include "Math.hpp"
#include <algorithm>
#include <sstream>

const int   Q3BspMap::s_tesselationLevel = 10;   // level of curved surface tesselation
const float Q3BspMap::s_worldScale       = 64.f; // scale down factor for the map

Q3BspMap::~Q3BspMap()
{
    delete [] entities.ents;
    delete [] visData.vecs;
    delete [] m_lightmapTextures;

    for (auto &it : m_patches)
        delete it;

    for (auto &it : m_renderBuffers.m_faceVBOs)
    {
        if (glIsBuffer(it.second.m_vertexBuffer))
        {
            glDeleteBuffers(1, &(it.second.m_vertexBuffer));
        }

        if (glIsBuffer(it.second.m_texcoordBuffer))
        {
            glDeleteBuffers(1, &(it.second.m_texcoordBuffer));
        }

        if (glIsBuffer(it.second.m_lightmapTexcoordBuffer))
        {
            glDeleteBuffers(1, &(it.second.m_lightmapTexcoordBuffer));
        }
    }

    for (auto &it : m_renderBuffers.m_patchVBOs)
    {
        for (auto &it2 : it.second)
        {
            if (glIsBuffer(it2.m_vertexBuffer))
            {
                glDeleteBuffers(1, &(it2.m_vertexBuffer));
            }

            if (glIsBuffer(it2.m_texcoordBuffer))
            {
                glDeleteBuffers(1, &(it2.m_texcoordBuffer));
            }

            if (glIsBuffer(it2.m_lightmapTexcoordBuffer))
            {
                glDeleteBuffers(1, &(it2.m_lightmapTexcoordBuffer));
            }
        }
    }

    if (glIsVertexArray(m_renderBuffers.m_vertexArray))
        glDeleteVertexArrays(1, &(m_renderBuffers.m_vertexArray));
}


void Q3BspMap::Init()
{
    m_missingTex = TextureManager::GetInstance()->LoadTexture("res/missing.png");

    // load textures
    LoadTextures();

    // load lightmaps
    LoadLightmaps();

    // create renderable faces and patches
    m_renderLeaves.reserve(leaves.size());

    for (const auto &l : leaves)
    {
        m_renderLeaves.push_back( Q3LeafRenderable() );
        m_renderLeaves.back().visCluster = l.cluster;
        m_renderLeaves.back().firstFace  = l.leafFace;
        m_renderLeaves.back().numFaces   = l.n_leafFaces;

        // create a bounding box
        m_renderLeaves.back().boundingBoxVertices[0] = Math::Vector3f( (float)l.mins.x, (float)l.mins.y,(float)l.mins.z );
        m_renderLeaves.back().boundingBoxVertices[1] = Math::Vector3f( (float)l.mins.x, (float)l.mins.y,(float)l.maxs.z );
        m_renderLeaves.back().boundingBoxVertices[2] = Math::Vector3f( (float)l.mins.x, (float)l.maxs.y,(float)l.mins.z );
        m_renderLeaves.back().boundingBoxVertices[3] = Math::Vector3f( (float)l.mins.x, (float)l.maxs.y,(float)l.maxs.z );
        m_renderLeaves.back().boundingBoxVertices[4] = Math::Vector3f( (float)l.maxs.x, (float)l.mins.y,(float)l.mins.z );
        m_renderLeaves.back().boundingBoxVertices[5] = Math::Vector3f( (float)l.maxs.x, (float)l.mins.y,(float)l.maxs.z );
        m_renderLeaves.back().boundingBoxVertices[6] = Math::Vector3f( (float)l.maxs.x, (float)l.maxs.y,(float)l.mins.z );
        m_renderLeaves.back().boundingBoxVertices[7] = Math::Vector3f( (float)l.maxs.x, (float)l.maxs.y,(float)l.maxs.z );

        for (int i = 0; i < 8; ++i)
        {
            m_renderLeaves.back().boundingBoxVertices[i].m_x /= Q3BspMap::s_worldScale;
            m_renderLeaves.back().boundingBoxVertices[i].m_y /= Q3BspMap::s_worldScale;
            m_renderLeaves.back().boundingBoxVertices[i].m_z /= Q3BspMap::s_worldScale;
        }
    }

    m_renderFaces.reserve(faces.size());

    // create the VAO
    glGenVertexArrays(1, &(m_renderBuffers.m_vertexArray));
    glBindVertexArray(m_renderBuffers.m_vertexArray);

    int faceArrayIdx  = 0;
    int patchArrayIdx = 0;

    for (const auto &f : faces)
    {
        m_renderFaces.push_back( Q3FaceRenderable() );

        //is it a patch?
        if (f.type == FaceTypePatch)
        {
            m_renderFaces.back().index = patchArrayIdx;
            CreatePatch(f);

            // generate necessary VBOs for current patch
            CreateBuffersForPatch(patchArrayIdx);
            ++patchArrayIdx;
        }
        else
        {
            m_renderFaces.back().index = faceArrayIdx;

            // generate necessary VBOs for current face
            CreateBuffersForFace(f, faceArrayIdx);
        }

        ++faceArrayIdx;
        m_renderFaces.back().type = f.type;
    }

    m_mapStats.totalVertices = vertices.size();
    m_mapStats.totalFaces    = faces.size();
    m_mapStats.totalPatches  = patchArrayIdx;

    // set the scale-down uniform
    glUniform1f(ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader).uniforms[WorldScaleFactor], 1.f / Q3BspMap::s_worldScale);
}


void Q3BspMap::OnRenderStart()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}


void Q3BspMap::Render()
{
    m_frustum.OnRender();

    m_mapStats.visiblePatches = 0;

    if (HasRenderFlag(Q3RenderShowWireframe))
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render visible faces
    const ShaderProgram &shader = ShaderManager::GetInstance()->UseShaderProgram(ShaderManager::BasicShader);

    GLuint vertexPosAttr = glGetAttribLocation(shader.id, "inVertex");
    GLuint texCoordAttr  = glGetAttribLocation(shader.id, "inTexCoord");
    GLuint lmapCoordAttr = glGetAttribLocation(shader.id, "inTexCoordLightmap");

    glBindVertexArray(m_renderBuffers.m_vertexArray);

    glEnableVertexAttribArray(vertexPosAttr);
    glEnableVertexAttribArray(texCoordAttr);
    glEnableVertexAttribArray(lmapCoordAttr);

    for (const auto &vf : m_visibleFaces)
    {
        // polygons and meshes are rendered in the same manner
        if (vf->type == FaceTypePolygon || vf->type == FaceTypeMesh)
        {
            RenderFace(vf->index);
        }

        // render all biquad patches that compose the curved surface
        if (vf->type == FaceTypePatch)
        {
            RenderPatch(vf->index);
            m_mapStats.visiblePatches++;
        }
    }

    glDisableVertexAttribArray(vertexPosAttr);
    glDisableVertexAttribArray(texCoordAttr);
    glDisableVertexAttribArray(lmapCoordAttr);
}


void Q3BspMap::OnRenderFinish()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}


// determine if a bsp cluster is visible from a given camera cluster
bool Q3BspMap::ClusterVisible(int cameraCluster, int testCluster) const
{
    if ((visData.vecs == NULL) || (cameraCluster < 0)) {
        return true;
    }

    int idx = (cameraCluster * visData.sz_vecs) + (testCluster >> 3);

    return (visData.vecs[idx] & (1 << (testCluster & 7))) != 0;
}


// determine which bsp leaf camera resides in
int Q3BspMap::FindCameraLeaf(const Math::Vector3f & cameraPosition) const
{
    int leafIndex = 0;

    while(leafIndex >= 0)
    {
        // children.x - front node; children.y - back node
        if(PointPlanePos( planes[nodes[leafIndex].plane].normal.x,
                          planes[nodes[leafIndex].plane].normal.y,
                          planes[nodes[leafIndex].plane].normal.z,
                          planes[nodes[leafIndex].plane].dist,
                          cameraPosition ) == Math::PointInFrontOfPlane )
        {
            leafIndex = nodes[leafIndex].children.x;
        }
        else
        {
            leafIndex = nodes[leafIndex].children.y;
        }
    }

    return ~leafIndex;
}


//Calculate which faces to draw given a camera position & view frustum
void Q3BspMap::CalculateVisibleFaces(const Math::Vector3f &cameraPosition)
{
    m_visibleFaces.clear();

    //calculate the camera leaf
    int cameraLeaf    = FindCameraLeaf(cameraPosition * Q3BspMap::s_worldScale);
    int cameraCluster = m_renderLeaves[cameraLeaf].visCluster;

    //loop through the leaves
    for (const auto &rl : m_renderLeaves)
    {
        //if the leaf is not in the PVS - skip it
        if( !HasRenderFlag( Q3RenderSkipPVS ) && !ClusterVisible(cameraCluster, rl.visCluster) )
            continue;

        //if this leaf does not lie in the frustum - skip it
        if( !HasRenderFlag( Q3RenderSkipFC ) && !m_frustum.BoxInFrustum(  rl.boundingBoxVertices ) )
            continue;

        //loop through faces in this leaf and them to visibility set
        for (int j = 0; j < rl.numFaces; ++j)
        {
            int idx = leafFaces[ rl.firstFace + j ].face;
            Q3FaceRenderable *face = &m_renderFaces[ leafFaces[ rl.firstFace + j ].face ];

            if(std::find( m_visibleFaces.begin(), m_visibleFaces.end(), face ) == m_visibleFaces.end() )
                m_visibleFaces.push_back( face );
        }
    }

    m_mapStats.visibleFaces = m_visibleFaces.size();
}


void Q3BspMap::LoadTextures()
{
    int numTextures = header.direntries[Textures].length / sizeof(Q3BspTextureLump);

    m_textures.resize( numTextures );

    // load the textures from file (determine wheter it's a jpg or tga)
    for (const auto &f : faces)
    {
        if (m_textures[f.texture])
            continue;

        std::string nameJPG = textures[f.texture].name;

        nameJPG.append(".jpg");

        m_textures[f.texture] = TextureManager::GetInstance()->LoadTexture(nameJPG.c_str());

        if (m_textures[f.texture] == NULL)
        {
            std::string nameTGA = textures[f.texture].name;
            nameTGA.append(".tga");

            m_textures[f.texture] = TextureManager::GetInstance()->LoadTexture(nameTGA.c_str());

            if (m_textures[f.texture] == NULL)
            {
                std::stringstream sstream;
                sstream << "Missing texture: " << nameTGA.c_str() << "\n";
                LOG_MESSAGE(sstream.str().c_str());
            }
        }
    }
}


void Q3BspMap::LoadLightmaps()
{
    m_lightmapTextures = new GLuint[lightMaps.size()];

    glGenTextures(lightMaps.size(), m_lightmapTextures);

    // optional: change gamma settings of the lightmaps (make them brighter)
    SetLightmapGamma(2.5f);

    for (size_t i = 0; i < lightMaps.size(); ++i)
    {
        glBindTexture(GL_TEXTURE_2D, m_lightmapTextures[i]);
        glEnable(GL_TEXTURE_2D); // to fix the "potential" bug on older ATI cards

        // Create texture from bsp lmap data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, lightMaps[i].map);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Create white texture for if no lightmap specified
    float white[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glGenTextures(1, &m_whiteTex);
    glBindTexture(GL_TEXTURE_2D, m_whiteTex);
    glEnable(GL_TEXTURE_2D); // to fix the "potential" bug on older ATI cards

    //Create texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGB, GL_FLOAT, white);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,     GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,     GL_TEXTURE_WRAP_T, GL_REPEAT);
}


// tweak lightmap gamma settings
void Q3BspMap::SetLightmapGamma(float gamma)
{
    for (size_t i = 0; i < lightMaps.size(); ++i)
    {
        for (int j = 0; j < 128 * 128; ++j)
        {
            float r, g, b;

            r = lightMaps[i].map[ j*3+0 ];
            g = lightMaps[i].map[ j*3+1 ];
            b = lightMaps[i].map[ j*3+2 ];

            r *= gamma / 255.0f;
            g *= gamma / 255.0f;
            b *= gamma / 255.0f;

            float scale = 1.0f;
            float temp;
            if( r > 1.0f && (temp = (1.0f/r) ) < scale) scale = temp;
            if( g > 1.0f && (temp = (1.0f/g) ) < scale) scale = temp;
            if( b > 1.0f && (temp = (1.0f/b) ) < scale) scale = temp;

            scale *= 255.0f;
            r *= scale;
            g *= scale;
            b *= scale;

            lightMaps[i].map[ j*3+0 ] = (GLubyte)r;
            lightMaps[i].map[ j*3+1 ] = (GLubyte)g;
            lightMaps[i].map[ j*3+2 ] = (GLubyte)b;
        }
    }
}


// create a Q3Bsp curved surface
void Q3BspMap::CreatePatch(const Q3BspFaceLump &f)
{
    Q3BspPatch *newPatch = new Q3BspPatch;

    newPatch->textureIdx  = f.texture;
    newPatch->lightmapIdx = f.lm_index;
    newPatch->width  = f.size.x;
    newPatch->height = f.size.y;

    int numPatchesWidth  = ( newPatch->width - 1  ) >> 1;
    int numPatchesHeight = ( newPatch->height - 1 ) >> 1;

    newPatch->quadraticPatches.resize( numPatchesWidth*numPatchesHeight );

    // generate biquadratic patches (components that make the curved surface)
    for (int y = 0; y < numPatchesHeight; ++y)
    {
        for (int x = 0; x < numPatchesWidth; ++x)
        {
            for (int row = 0; row < 3; ++row)
            {
                for (int col = 0; col < 3; ++col)
                {
                    int patchIdx = y * numPatchesWidth + x;
                    int cpIdx = row * 3 + col;
                    newPatch->quadraticPatches[ patchIdx ].controlPoints[ cpIdx ] = vertices[ f.vertex +
                                                                                              ( y * 2 * newPatch->width + x * 2 ) +
                                                                                              row * newPatch->width + col ];
                }
            }

            newPatch->quadraticPatches[ y * numPatchesWidth + x ].Tesselate( Q3BspMap::s_tesselationLevel );
        }
    }

    m_patches.push_back(newPatch);
}


// render regular faces (polygons + meshes)
void Q3BspMap::RenderFace(int idx)
{
    const ShaderProgram &shader = ShaderManager::GetInstance()->GetActiveShader();
    GLuint vertexPosAttr = glGetAttribLocation(shader.id, "inVertex");
    GLuint texCoordAttr  = glGetAttribLocation(shader.id, "inTexCoord");
    GLuint lmapCoordAttr = glGetAttribLocation(shader.id, "inTexCoordLightmap");

    // bind vertices
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_vertexBuffer);
    glVertexAttribPointer(vertexPosAttr, 3, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

    // bind texture coords
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_texcoordBuffer);
    glVertexAttribPointer(texCoordAttr, 2, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

    // bind lightmap coords
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_lightmapTexcoordBuffer);
    glVertexAttribPointer(lmapCoordAttr, 2, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

    // bind primary texture
    glActiveTexture(GL_TEXTURE0);
    if (m_textures[faces[idx].texture])
        TextureManager::GetInstance()->BindTexture(m_textures[faces[idx].texture]);
    else
    {
        if (HasRenderFlag(Q3RenderSkipMissingTex))
        {
            return;
        }

        // render objects with missing textures without culling
        TextureManager::GetInstance()->BindTexture(m_missingTex);
        glDisable(GL_CULL_FACE);
    }

    // bind a generic white texture if there's no lightmap for this surface
    glActiveTexture(GL_TEXTURE1);
    if (faces[idx].lm_index >= 0)
        glBindTexture(GL_TEXTURE_2D, m_lightmapTextures[faces[idx].lm_index]);
    else
        glBindTexture(GL_TEXTURE_2D, m_whiteTex);


    glDrawElements(GL_TRIANGLES, faces[idx].n_meshverts, GL_UNSIGNED_INT, &meshVertices[faces[idx].meshvert]);

    // reenable culling in case it was disabled by missing texture
    glEnable(GL_CULL_FACE);
}


// render curved surface
void Q3BspMap::RenderPatch(int idx)
{
    // bind primary texture
    glActiveTexture(GL_TEXTURE0);
    if (m_textures[m_patches[idx]->textureIdx])
        TextureManager::GetInstance()->BindTexture(m_textures[m_patches[idx]->textureIdx]);
    else
        TextureManager::GetInstance()->BindTexture(m_missingTex);

    // bind a generic white texture if there's no lightmap for this surface
    glActiveTexture(GL_TEXTURE1);
    if (m_patches[idx]->lightmapIdx >= 0)
        glBindTexture(GL_TEXTURE_2D, m_lightmapTextures[m_patches[idx]->lightmapIdx]);
    else
        glBindTexture(GL_TEXTURE_2D, m_whiteTex);

    int numPatches = m_patches[idx]->quadraticPatches.size();

    const ShaderProgram &shader = ShaderManager::GetInstance()->GetActiveShader();
    GLuint vertexPosAttr = glGetAttribLocation(shader.id, "inVertex");
    GLuint texCoordAttr  = glGetAttribLocation(shader.id, "inTexCoord");
    GLuint lmapCoordAttr = glGetAttribLocation(shader.id, "inTexCoordLightmap");

    for (int i = 0; i < numPatches; ++i)
    {
        // bind vertices
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_vertexBuffer);
        glVertexAttribPointer(vertexPosAttr, 3, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

        // bind texture coords
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_texcoordBuffer);
        glVertexAttribPointer(texCoordAttr, 2, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

        // bind lightmap coords
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_lightmapTexcoordBuffer);
        glVertexAttribPointer(lmapCoordAttr, 2, GL_FLOAT, GL_FALSE, sizeof(Q3BspVertexLump), (void*)(0));

        m_patches[idx]->quadraticPatches[i].Render();
    }
}


void Q3BspMap::CreateBuffersForFace(const Q3BspFaceLump &face, int idx)
{
    glGenBuffers(1, &(m_renderBuffers.m_faceVBOs[idx].m_vertexBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * face.n_vertexes, &(vertices[face.vertex].position), GL_STATIC_DRAW);

    glGenBuffers(1, &(m_renderBuffers.m_faceVBOs[idx].m_texcoordBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_texcoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * face.n_vertexes, &(vertices[face.vertex].texcoord[0]), GL_STATIC_DRAW);

    glGenBuffers(1, &(m_renderBuffers.m_faceVBOs[idx].m_lightmapTexcoordBuffer));
    glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_faceVBOs[idx].m_lightmapTexcoordBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * face.n_vertexes, &(vertices[face.vertex].texcoord[1]), GL_STATIC_DRAW);
}


void Q3BspMap::CreateBuffersForPatch(int idx)
{
    int numPatches = m_patches[idx]->quadraticPatches.size();

    for (int i = 0; i < numPatches; i++)
    {
        m_renderBuffers.m_patchVBOs[idx].push_back(FaceBuffers());

        int numVerts = m_patches[idx]->quadraticPatches[i].m_vertices.size();

        glGenBuffers(1, &(m_renderBuffers.m_patchVBOs[idx][i].m_vertexBuffer));
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * numVerts, &(m_patches[idx]->quadraticPatches[i].m_vertices[0].position), GL_STATIC_DRAW);

        glGenBuffers(1, &(m_renderBuffers.m_patchVBOs[idx][i].m_texcoordBuffer));
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_texcoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * numVerts, &(m_patches[idx]->quadraticPatches[i].m_vertices[0].texcoord[0]), GL_STATIC_DRAW);

        glGenBuffers(1, &(m_renderBuffers.m_patchVBOs[idx][i].m_lightmapTexcoordBuffer));
        glBindBuffer(GL_ARRAY_BUFFER, m_renderBuffers.m_patchVBOs[idx][i].m_lightmapTexcoordBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Q3BspVertexLump) * numVerts, &(m_patches[idx]->quadraticPatches[i].m_vertices[0].texcoord[1]), GL_STATIC_DRAW);
    }
}
