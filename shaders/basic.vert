#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoords;
out vec4 fragPosLightSpace; // <--- Variabila esentiala pentru umbre

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix; // <--- Matricea de transformare a umbrei

void main() 
{
    gl_Position = projection * view * model * vec4(vPosition, 1.0f);
    fPosition = vPosition;
    fNormal = vNormal;
    fTexCoords = vTexCoords;

    // Calculam pozitia vertexului vazuta de soare
    fragPosLightSpace = lightSpaceTrMatrix * model * vec4(vPosition, 1.0f);
}