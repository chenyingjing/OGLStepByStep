precision mediump float;

varying vec2 fragTexCoord;                                                                
uniform sampler2D gShadowMap;                                                       


void main()                                                                         
{                                                                                   
    float Depth = texture2D(gShadowMap, fragTexCoord).x;                               
    Depth = 1.0 - (1.0 - Depth) * 25.0;
    gl_FragColor = vec4(vec3(Depth), 1);
    //FragColor = vec4(vec3(Depth), 1);
}
