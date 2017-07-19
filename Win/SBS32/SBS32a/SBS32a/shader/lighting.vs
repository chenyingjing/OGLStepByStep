#version 410 core                                                                               

in vec3 vert;
in vec2 vertTexCoord;
in vec3 vertNormal;

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
