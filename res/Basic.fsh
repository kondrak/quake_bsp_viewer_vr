#version 410

uniform sampler2D sTexture;
uniform sampler2D sLightmap;

uniform int renderLightmaps = 0;
uniform int useLightmaps    = 1;
uniform int useAlphaTest    = 0;

layout(location = 3) in vec2 TexCoord;
layout(location = 4) in vec2 TexCoordLightmap;

out vec4 fragmentColor;

void main()
{
    vec4 baseTex  = texture(sTexture, TexCoord);
    vec4 lightMap = texture(sLightmap, TexCoordLightmap);
      
    if(renderLightmaps == 1)
    {
        fragmentColor = lightMap * 1.2;
    }
    else
    {
        if(useLightmaps == 0)
        {
            lightMap = vec4(1.0, 1.0, 1.0, 1.0);
        }        

        if(useAlphaTest == 1)
        {
            if(baseTex.a >= 0.05)
                fragmentColor = baseTex * lightMap * 2.0; // make the output more vivid
            else
                discard;
        }
        else
        {
            fragmentColor = baseTex * lightMap * 2.0; // make the output more vivid
        }
    }   
}
