#version 330

layout (location = 0) in vec3 vert;

uniform mat4 model;
uniform mat4 camera;

void main()
{
	gl_Position = camera * model * vec4(vert, 1);
}
