#ifndef Q3BSPPATCH_INCLUDED
#define Q3BSPPATCH_INCLUDED

#include "q3bsp/Q3BspMap.hpp"


// Quake III BSP curved surface component ( biquadratic (3x3) patch )
class Q3BspBiquadPatch
{
public:
    Q3BspBiquadPatch() : m_tesselationLevel(0), 
                         m_trianglesPerRow(NULL), 
                         m_rowIndexPointers(NULL)
    {
    }

    ~Q3BspBiquadPatch()
    {
        delete [] m_trianglesPerRow;
        delete [] m_rowIndexPointers;
    }

    void Tesselate(int tessLevel);      // perform tesselation 
    void Render();

    Q3BspVertexLump controlPoints[9];
    std::vector<Q3BspVertexLump> m_vertices;
private:
    int                          m_tesselationLevel;    
    std::vector<unsigned int>    m_indices;
    int*                         m_trianglesPerRow;   // store as pointer arrays for easier access by GL functions
    unsigned int**               m_rowIndexPointers;  //  - " -
};


// Quake III BSP curved surface (an array of biquadratic patches)
struct Q3BspPatch
{
    int textureIdx;   // surface texture index
    int lightmapIdx;  // surface lightmap index
    int width;
    int height;

    std::vector<Q3BspBiquadPatch> quadraticPatches;
};

#endif