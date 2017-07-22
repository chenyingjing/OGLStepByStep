#version 410 core                                                                               

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 vertTexCoord;
layout (location = 2) in vec3 vertNormal;

uniform mat4 model;
uniform mat4 camera;

out vec3 WorldPos_FS_in;
out vec2 TexCoord_FS_in;
out vec3 Normal_FS_in;

void main()
{
    WorldPos_FS_in = vert;
    TexCoord_FS_in = vertTexCoord;
    Normal_FS_in   = vertNormal;

	gl_Position = camera * model * vec4(vert, 1);
}
