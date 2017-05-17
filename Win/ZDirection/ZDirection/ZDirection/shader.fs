#version 330

in vec3 Color0;

out vec4 FragColor;

void main()
{
    FragColor = vec4(Color0, 1.0);
    //FragColor = vec4(1, 0, 0, 1.0);
}
