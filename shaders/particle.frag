#version 410 core

in vec2 fTexCoords;
out vec4 fColor;

// Nu mai avem sampler2D (textura), doar opacitatea
uniform float opacity; 

void main()
{
    // calculare distanta pixelului fata de centrul patratului (0.5, 0.5)
    float dist = distance(fTexCoords, vec2(0.5));

    // daca suntem in afara cercului (raza 0.5) ==> pixelul invizibil
    if(dist > 0.5)
        discard;

    // trecere lina (gradient) de la centru spre margine
    // smoothstep va face marginile "pufoase" nu ascutite
    float alpha = 1.0 - smoothstep(0.0, 0.5, dist);

    // setare culoarea gri cu transparenta calculata
    // inmulit cu 'opacity' ca sa dispara in timp (cand moare particula)
    fColor = vec4(0.2, 0.2, 0.2, alpha * opacity);
}