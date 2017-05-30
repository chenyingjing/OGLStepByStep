#version 330                                                                        
                                                                                    
in vec2 fragTexCoord;                                                                
uniform sampler2D gShadowMap;                                                       

//out vec4 finalColor;
out vec4 FragColor;

void main()                                                                         
{                                                                                   
    float Depth = texture(gShadowMap, fragTexCoord).x;                               
    Depth = 1.0 - (1.0 - Depth) * 25.0;
    //finalColor = vec4(vec3(Depth), 1);
    FragColor = vec4(vec3(Depth), 1);
}
