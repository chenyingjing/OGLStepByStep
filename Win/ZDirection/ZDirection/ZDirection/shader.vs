#version 330

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Color;

out vec3 Color0;

void main()
{
    gl_Position = vec4(0.5 * Position.x, 0.5 * Position.y, Position.z, 1.0);
	Color0 = Color;
}