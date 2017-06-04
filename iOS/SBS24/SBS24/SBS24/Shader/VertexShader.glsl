uniform mat4 camera;
uniform mat4 model;

attribute vec3 vert;
attribute vec2 vertTexCoord;
attribute vec3 vertNormal;

varying vec3 fragVert;
varying vec2 fragTexCoord;
varying vec3 fragNormal;

void main(void)
{
    fragTexCoord = vertTexCoord;
    fragNormal = vertNormal;
    fragVert = vert;
    
    gl_Position = camera * model * vec4(vert, 1);
}

