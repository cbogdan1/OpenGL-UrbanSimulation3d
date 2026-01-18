#version 410 core

in vec3 Normal;
in vec3 Position;

out vec4 fColor;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    // Calculam vectorul de incidenta (de la camera la obiect)
    vec3 I = normalize(Position - cameraPos);
    
    // Calculam vectorul de reflexie
    vec3 R = reflect(I, normalize(Normal));
    
    // Luam culoarea din skybox de la coordonata vectorului R
    fColor = vec4(texture(skybox, R).rgb, 1.0);
}