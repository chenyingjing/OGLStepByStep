#version 330

layout (location = 0) in vec3 vert;
layout (location = 1) in vec2 vertTexCoord;
layout (location = 2) in vec3 vertNormal;
layout (location = 3) in mat4 WVP; 
layout (location = 7) in mat4 World;

//uniform mat4 model;
//uniform mat4 camera;

out vec3 WorldPos_FS_in;
out vec2 TexCoord_FS_in;
out vec3 Normal_FS_in;
flat out int InstanceID;

void main()
{
    WorldPos_FS_in = (World * vec4(vert, 1.0)).xyz;;
    TexCoord_FS_in = vertTexCoord;
    Normal_FS_in   = (World * vec4(vertNormal, 0.0)).xyz;

	gl_Position = WVP * vec4(vert, 1);
	InstanceID = gl_InstanceID;
}
