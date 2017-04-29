attribute vec4 vPosition;

void main(void)
{
    gl_Position = vPosition;
    gl_PointSize = 2.0;
    //gl_Position = vec4(vPosition.xyz, 1);
    //gl_Position = vec4(0, 0, 0, 1);
}

