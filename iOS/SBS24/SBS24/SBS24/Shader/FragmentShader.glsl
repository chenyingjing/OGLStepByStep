
precision highp float;

uniform highp mat4 model;
uniform mat3 normalMatrix;

uniform vec3 cameraPosition;

uniform sampler2D materialTex;
uniform sampler2D gShadowMap;
uniform float materialShininess;
uniform vec3 materialSpecularColor;
uniform float delta;

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

varying vec2 fragTexCoord;
varying vec3 fragNormal;
varying vec3 fragVert;
varying vec4 lightSpacePos;


vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfacePos, vec3 surfaceToCamera, vec4 LightSpacePos);

float CalcShadowFactor(vec4 LightSpacePos)
{
    vec3 ProjCoords = LightSpacePos.xyz / LightSpacePos.w;
    vec2 UVCoords;
    UVCoords.x = 0.5 * ProjCoords.x + 0.5;
    UVCoords.y = 0.5 * ProjCoords.y + 0.5;
    float z = 0.5 * ProjCoords.z + 0.5;
    
    float Depth = texture2D(gShadowMap, UVCoords).x;
    if (Depth < z - delta)
        //return 0.01;
        return 0.0;
    else
        return 1.0;
}

void main() {
    vec3 normal = normalize(normalMatrix * fragNormal);
    vec3 surfacePos = vec3(model * vec4(fragVert, 1));
    vec4 surfaceColor = texture2D(materialTex, fragTexCoord);
    vec3 surfaceToCamera = normalize(cameraPosition - surfacePos); //also a unit
    
    vec3 linearColor = vec3(0);
    for(int i = 0; i < numLights; ++i){
        linearColor += ApplyLight(allLights[i], surfaceColor.rgb, normal, surfacePos, surfaceToCamera, lightSpacePos);
    }
    
//    vec3 gamma = vec3(1.0/2.2);
//    gl_FragColor = vec4(pow(linearColor, gamma), surfaceColor.a);
    gl_FragColor = vec4(linearColor, surfaceColor.a);
}

vec3 ApplyLight(Light light, vec3 surfaceColor, vec3 normal, vec3 surfacePos, vec3 surfaceToCamera, vec4 LightSpacePos) {
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
        attenuation = 1.0 / (1.0 + light.attenuation * pow(distanceToLight, 2.0));
        
        //cone restrictions (affects attenuation)
        float spotFactor = dot(-surfaceToLight, normalize(light.coneDirection));
        float lightToSurfaceAngle = degrees(acos(spotFactor));
        if(lightToSurfaceAngle > light.coneAngle){
            attenuation = 0.0;
        } else {
            float cutoff = cos(radians(light.coneAngle));
            attenuation *= (1.0 - (1.0 - spotFactor) * 1.0/(1.0 - cutoff));
            float ShadowFactor = CalcShadowFactor(LightSpacePos);
            attenuation *= ShadowFactor;
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
