#version 330
                                                                                            

//uniform mat4 model;
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

uniform vec4 gColor[4];


in vec2 TexCoord_FS_in;
in vec3 Normal_FS_in;
in vec3 WorldPos_FS_in;
flat in int InstanceID;

out vec4 finalColor;



vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfacePos, vec3 surfaceToCamera);

void main() {
    //vec3 normal = normalize(transpose(inverse(mat3(model))) * Normal_FS_in);
    vec3 normal = normalize(Normal_FS_in);
    //vec3 surfacePos = vec3(model * vec4(WorldPos_FS_in, 1));
    vec3 surfacePos = WorldPos_FS_in;
    //vec4 surfaceColor = texture(materialTex, fragTexCoord);
    vec4 surfaceColor = texture(materialTex, TexCoord_FS_in);
    vec3 surfaceToCamera = normalize(cameraPosition - surfacePos); //also a unit
    
    vec3 linearColor = vec3(0);
    for(int i = 0; i < numLights; ++i){
        linearColor += ApplyLight(allLights[i], surfaceColor.rgb, normal, surfacePos, surfaceToCamera);
    }
    
    vec3 gamma = vec3(1.0/2.2);
    finalColor = vec4(pow(linearColor, gamma), surfaceColor.a) * gColor[InstanceID % 4];
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


