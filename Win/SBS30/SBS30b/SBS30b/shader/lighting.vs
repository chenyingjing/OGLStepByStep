#version 410 core                                                                               

in vec3 vert;
in vec2 vertTexCoord;
in vec3 vertNormal;

uniform mat4 model;

out vec3 WorldPos_CS_in;
out vec2 TexCoord_CS_in;
out vec3 Normal_CS_in;

void main()
{
    WorldPos_CS_in = (model * vec4(vert, 1.0)).xyz;
    TexCoord_CS_in = vertTexCoord;
    Normal_CS_in   = (model * vec4(vertNormal, 0.0)).xyz;
}
