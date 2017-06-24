#version 330

in vec3 vert;

//uniform mat4 camera;
//uniform mat4 model;

//in vec2 vertTexCoord;

//out vec3 fragVert;
//out vec2 fragTexCoord;
//out vec3 fragNormal;

void main() {
    //fragTexCoord = vertTexCoord;
    //fragVert = vert;
    
    gl_Position = vec4(vert, 1);
}