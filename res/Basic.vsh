#version 410

uniform mat4  ModelViewProjectionMatrix;
uniform float worldScaleFactor;

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec2 inTexCoordLightmap;

layout(location = 3) out vec2 TexCoord;
layout(location = 4) out vec2 TexCoordLightmap;

void main()
{
    gl_Position = ModelViewProjectionMatrix * vec4(inVertex * worldScaleFactor, 1.0);    
	TexCoord    = inTexCoord; 
    TexCoordLightmap = inTexCoordLightmap;
}
 