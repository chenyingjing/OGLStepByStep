#version 410 core                                                                           
                                                                                            
//const int MAX_POINT_LIGHTS = 2;                                                             
//const int MAX_SPOT_LIGHTS = 2;                                                              
//                                                                                            
//in vec2 TexCoord_FS_in;                                                                     
//in vec3 Normal_FS_in;                                                                       
//in vec3 WorldPos_FS_in;                                                                     
//                                                                                            
//out vec4 FragColor;                                                                         
//                                                                                            
//struct BaseLight                                                                            
//{                                                                                           
//    vec3 Color;                                                                             
//    float AmbientIntensity;                                                                 
//    float DiffuseIntensity;                                                                 
//};                                                                                          
//                                                                                            
//struct DirectionalLight                                                                     
//{                                                                                           
//    BaseLight Base;                                                                  
//    vec3 Direction;                                                                         
//};                                                                                          
//                                                                                            
//struct Attenuation                                                                          
//{                                                                                           
//    float Constant;                                                                         
//    float Linear;                                                                           
//    float Exp;                                                                              
//};                                                                                          
//                                                                                            
//struct PointLight                                                                           
//{                                                                                           
//    BaseLight Base;                                                                  
//    vec3 Position;                                                                          
//    Attenuation Atten;                                                                      
//};                                                                                          
//                                                                                            
//struct SpotLight                                                                            
//{                                                                                           
//    PointLight Base;                                                                 
//    vec3 Direction;                                                                         
//    float Cutoff;                                                                           
//};                                                                                          
//                                                                                            
//uniform int gNumPointLights;                                                                
//uniform int gNumSpotLights;                                                                 
//uniform DirectionalLight gDirectionalLight;                                                 
//uniform PointLight gPointLights[MAX_POINT_LIGHTS];                                          
//uniform SpotLight gSpotLights[MAX_SPOT_LIGHTS];                                             
//uniform sampler2D gColorMap;                                                                
//uniform vec3 gEyeWorldPos;                                                                  
//uniform float gMatSpecularIntensity;                                                        
//uniform float gSpecularPower;                                                               
//                                                                                            
//vec4 CalcLightInternal(BaseLight Light, vec3 LightDirection, vec3 Normal)            
//{                                                                                           
//    vec4 AmbientColor = vec4(Light.Color * Light.AmbientIntensity, 1.0f);
//    float DiffuseFactor = dot(Normal, -LightDirection);                                     
//                                                                                            
//    vec4 DiffuseColor  = vec4(0, 0, 0, 0);                                                  
//    vec4 SpecularColor = vec4(0, 0, 0, 0);                                                  
//                                                                                            
//    if (DiffuseFactor > 0) {                                                                
//        DiffuseColor = vec4(Light.Color * Light.DiffuseIntensity * DiffuseFactor, 1.0f);
//                                                                                            
//        vec3 VertexToEye = normalize(gEyeWorldPos - WorldPos_FS_in);                        
//        vec3 LightReflect = normalize(reflect(LightDirection, Normal));                     
//        float SpecularFactor = dot(VertexToEye, LightReflect);                              
//        if (SpecularFactor > 0) {                                                           
//            SpecularFactor = pow(SpecularFactor, gSpecularPower);                               
//            SpecularColor = vec4(Light.Color * gMatSpecularIntensity * SpecularFactor, 1.0f);
//        }                                                                                   
//    }                                                                                       
//                                                                                            
//    return (AmbientColor + DiffuseColor + SpecularColor);                                   
//}                                                                                           
//                                                                                            
//vec4 CalcDirectionalLight(vec3 Normal)                                                      
//{                                                                                           
//    return CalcLightInternal(gDirectionalLight.Base, gDirectionalLight.Direction, Normal);  
//}                                                                                           
//                                                                                            
//vec4 CalcPointLight(PointLight l, vec3 Normal)                                       
//{                                                                                           
//    vec3 LightDirection = WorldPos_FS_in - l.Position;                                      
//    float Distance = length(LightDirection);                                                
//    LightDirection = normalize(LightDirection);                                             
//                                                                                            
//    vec4 Color = CalcLightInternal(l.Base, LightDirection, Normal);                         
//    float Attenuation =  l.Atten.Constant +                                                 
//                         l.Atten.Linear * Distance +                                        
//                         l.Atten.Exp * Distance * Distance;                                 
//                                                                                            
//    return Color / Attenuation;                                                             
//}                                                                                           
//                                                                                            
//vec4 CalcSpotLight(SpotLight l, vec3 Normal)                                         
//{                                                                                           
//    vec3 LightToPixel = normalize(WorldPos_FS_in - l.Base.Position);                        
//    float SpotFactor = dot(LightToPixel, l.Direction);                                      
//                                                                                            
//    if (SpotFactor > l.Cutoff) {                                                            
//        vec4 Color = CalcPointLight(l.Base, Normal);                                        
//        return Color * (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - l.Cutoff));                   
//    }                                                                                       
//    else {                                                                                  
//        return vec4(0,0,0,0);                                                               
//    }                                                                                       
//}                                                                                           
//                                                                                            
//void main()                                                                                 
//{                                                                                           
//    vec3 Normal = normalize(Normal_FS_in);                                                  
//    vec4 TotalLight = CalcDirectionalLight(Normal);                                         
//                                                                                            
//    for (int i = 0 ; i < gNumPointLights ; i++) {                                           
//        TotalLight += CalcPointLight(gPointLights[i], Normal);                              
//    }                                                                                       
//                                                                                            
//    for (int i = 0 ; i < gNumSpotLights ; i++) {                                            
//        TotalLight += CalcSpotLight(gSpotLights[i], Normal);                                
//    }                                                                                       
//                                                                                            
//    FragColor = texture(gColorMap, TexCoord_FS_in.xy) * TotalLight;                         
//}

uniform mat4 model;
uniform vec3 cameraPosition;

uniform sampler2D materialTex;
uniform sampler2D gShadowMap;
uniform float materialShininess;
uniform vec3 materialSpecularColor;

// array of lights
#define MAX_LIGHTS 10
uniform int numLights;
uniform struct Light {
    vec4 position;
    vec3 intensities; //a.k.a the color of the light
    float attenuation;
    float ambientCoefficient;
    float coneAngle;
    vec3 coneDirection;
} allLights[MAX_LIGHTS];

in vec2 TexCoord_FS_in;
in vec3 Normal_FS_in;
in vec3 WorldPos_FS_in;

out vec4 finalColor;



vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfacePos, vec3 surfaceToCamera);

void main() {
    //vec3 normal = normalize(transpose(inverse(mat3(model))) * fragNormal);
    vec3 normal = normalize(Normal_FS_in);
    //vec3 surfacePos = vec3(model * vec4(fragVert, 1));
    vec3 surfacePos = WorldPos_FS_in;
    //vec4 surfaceColor = texture(materialTex, fragTexCoord);
    vec4 surfaceColor = texture(materialTex, TexCoord_FS_in);
    vec3 surfaceToCamera = normalize(cameraPosition - surfacePos); //also a unit
    
    vec3 linearColor = vec3(0);
    for(int i = 0; i < numLights; ++i){
        linearColor += ApplyLight(allLights[i], surfaceColor.rgb, normal, surfacePos, surfaceToCamera);
    }
    
    vec3 gamma = vec3(1.0/2.2);
    finalColor = vec4(pow(linearColor, gamma), surfaceColor.a);
}

vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfacePos, vec3 surfaceToCamera) {
    vec3 surfaceToLight;
    float attenuation = 1.0;
    if(light.position.w == 0.0) {
        //directional light
        surfaceToLight = normalize(light.position.xyz);
        attenuation = 1.0; //no attenuation for directional lights
    } else {
        //point light
        surfaceToLight = normalize(light.position.xyz - surfacePos);
        float distanceToLight = length(light.position.xyz - surfacePos);
        attenuation = 1.0 / (1.0 + light.attenuation * pow(distanceToLight, 2));
        
        //cone restrictions (affects attenuation)
        float spotFactor = dot(-surfaceToLight, normalize(light.coneDirection));
        float lightToSurfaceAngle = degrees(acos(spotFactor));
        if(lightToSurfaceAngle > light.coneAngle){
            attenuation = 0.0;
        } else {
            float cutoff = cos(radians(light.coneAngle));
            attenuation *= (1.0 - (1.0 - spotFactor) * 1.0/(1.0 - cutoff));
        }
    }
    
    //ambient
    vec3 ambient = light.ambientCoefficient * surfaceColor.rgb * light.intensities;
    
    //diffuse
    float diffuseCoefficient = max(0.0, dot(normal, surfaceToLight));
    vec3 diffuse = diffuseCoefficient * surfaceColor.rgb * light.intensities;
    
    //specular
    float specularCoefficient = 0.0;
    if(diffuseCoefficient > 0.0)
        specularCoefficient = pow(max(0.0, dot(surfaceToCamera, reflect(-surfaceToLight, normal))), materialShininess);
    vec3 specular = specularCoefficient * materialSpecularColor * light.intensities;
    
    //linear color (color before gamma correction)
    return ambient + attenuation*(diffuse + specular);
}


