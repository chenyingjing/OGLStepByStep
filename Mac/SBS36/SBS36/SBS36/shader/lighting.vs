#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 vertTexCoord;
layout (location = 2) in vec3 vertNormal;

uniform mat4 model;
uniform mat4 camera;


out vec2 TexCoord0;
out vec3 Normal0;
out vec3 WorldPos0;


void main()
{
    gl_Position    = camera * model * vec4(vert, 1.0);
    TexCoord0      = vertTexCoord;
    Normal0        = (model * vec4(vertNormal, 0.0)).xyz;
    WorldPos0      = (model * vec4(vert, 1.0)).xyz;
}
