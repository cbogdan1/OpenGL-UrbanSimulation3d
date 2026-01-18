#version 410 core

#define NR_POINT_LIGHTS 6
#define NR_SPOT_LIGHTS 2  // Definim 2 faruri

in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fragPosLightSpace;

out vec4 fColor;

// --- STRUCTURI ---
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;      // Unghi interior
    float outerCutOff; // Unghi exterior (fade)
  
    float constant;
    float linear;
    float quadratic;
  
    vec3 color;
};

// --- UNIFORME ---
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

// 1. Directionala (Soare)
uniform vec3 lightDir;
uniform vec3 lightColorDir;

// 2. Punctiforme (Becuri stradale)
uniform vec3 lightPos[NR_POINT_LIGHTS];
uniform vec3 lightColorPoint[NR_POINT_LIGHTS];

// 3. Spot (Faruri)
uniform SpotLight spotLight[NR_SPOT_LIGHTS];

// Texturi
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Constante Material
float ambientStrength = 0.2f;
uniform float specularStrength;
float shininess = 32.0f;

// Constante Atenuare implicite (folosite la point lights)
float constant = 1.0f;
float linear = 0.09f;
float quadratic = 0.032f;

// --- FUNCTII ---

float computeShadow(vec3 normal, vec3 lightDir)
{
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    normalizedCoords = normalizedCoords * 0.5 + 0.5;
    if (normalizedCoords.z > 1.0f) return 0.0f;
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
    float currentDepth = normalizedCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    return shadow;
}

// Calcul Lumina Directionala (Soare)
vec3 computeDirLight(vec3 normalEye, vec4 fPosEye, vec3 colorFromTexture, float shadow) {
    vec3 lightDirN = normalize(view * vec4(lightDir, 0.0f)).xyz;
    vec3 viewDirN = normalize(-fPosEye.xyz);

    vec3 ambient = ambientStrength * lightColorDir;
    vec3 diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColorDir;
    
    vec3 halfwayDir = normalize(lightDirN + viewDirN);
    float specCoeff = pow(max(dot(normalEye, halfwayDir), 0.0f), shininess);
    vec3 specular = specularStrength * specCoeff * lightColorDir;

    return (ambient + (1.0f - shadow) * diffuse) * colorFromTexture + (1.0f - shadow) * specular;
}

// Calcul Lumina Punctiforma (Stalpi)
vec3 computePointLight(vec3 pos, vec3 color, vec3 normalEye, vec4 fPosEye, vec3 colorFromTexture) {
    vec4 lightPosEye = view * vec4(pos, 1.0f);
    vec3 lightDirN = normalize(lightPosEye.xyz - fPosEye.xyz);
    vec3 viewDirN = normalize(-fPosEye.xyz);

    float dist = length(lightPosEye.xyz - fPosEye.xyz);
    float attenuation = 1.0 / (constant + linear * dist + quadratic * (dist * dist));

    vec3 ambient = 0.05f * color;
    vec3 diffuse = max(dot(normalEye, lightDirN), 0.0f) * color;
    
    vec3 halfwayDir = normalize(lightDirN + viewDirN);
    float specCoeff = pow(max(dot(normalEye, halfwayDir), 0.0f), shininess);
    vec3 specular = specularStrength * specCoeff * color;

    return (ambient + diffuse + specular) * attenuation * colorFromTexture;
}

// Calcul Lumina Spot (Faruri)
vec3 computeSpotLight(SpotLight light, vec3 normalEye, vec4 fPosEye, vec3 colorFromTexture) {
    // Transformam pozitia si directia in Eye Space pentru a fi compatibil cu restul calculelor
    vec4 lightPosEye = view * vec4(light.position, 1.0f);
    vec3 lightDirFromObj = normalize(lightPosEye.xyz - fPosEye.xyz);
    vec3 viewDirN = normalize(-fPosEye.xyz);
    
    // Directia spot-ului trebuie transformata si ea in Eye Space (folosind doar rotatia din view)
    vec3 spotDirEye = normalize(mat3(view) * light.direction);

    // 1. Difuza
    float diff = max(dot(normalEye, lightDirFromObj), 0.0);
    
    // 2. Speculara (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDirFromObj + viewDirN);
    float spec = pow(max(dot(normalEye, halfwayDir), 0.0), shininess);
    
    // 3. Atenuare (Distanta)
    float distance = length(lightPosEye.xyz - fPosEye.xyz);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
    
    // 4. Intensitate Spot (Conul)
    // Calculam unghiul dintre vectorul fragmentului si directia spotului
    float theta = dot(lightDirFromObj, normalize(-spotDirEye)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Combinare
    vec3 ambient = 0.0f * light.color; // Farurile nu prea au ambient
    vec3 diffuse = light.color * diff;
    vec3 specular = light.color * spec * specularStrength; // Folosim specularStrength global
    
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    
    return (ambient + diffuse + specular) * colorFromTexture;
}

float computeFog(vec4 fPosEye)
{
    float fogDensity = 0.005f; // Ajusteaza densitatea aici
    float fragmentDistance = length(fPosEye.xyz);
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
    return clamp(fogFactor, 0.0f, 1.0f);
}

void main() {
    vec4 fPosEye = view * model * vec4(fPosition, 1.0f);
    vec3 normalEye = normalize(normalMatrix * fNormal);
    vec3 texColor = texture(diffuseTexture, fTexCoords).rgb;

    // 1. Soare (Directional)
    vec3 lightDirN = normalize(view * vec4(lightDir, 0.0f)).xyz;
    float shadow = computeShadow(normalEye, lightDirN);
    vec3 finalColor = computeDirLight(normalEye, fPosEye, texColor, shadow);
    
    // 2. Becuri Stradale (Point Lights)
    for(int i = 0; i < NR_POINT_LIGHTS; i++) {
        finalColor += computePointLight(lightPos[i], lightColorPoint[i], normalEye, fPosEye, texColor);
    }

    // 3. Faruri Masina (Spot Lights)
    for(int i = 0; i < NR_SPOT_LIGHTS; i++) {
         finalColor += computeSpotLight(spotLight[i], normalEye, fPosEye, texColor);
    }

    // 4. Ceata
    float fogFactor = computeFog(fPosEye);
    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);

    fColor = mix(fogColor, vec4(min(finalColor, 1.0f), 1.0f), fogFactor);
}