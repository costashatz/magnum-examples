// #version 430

layout(location = 0) out vec3 gAlbedo;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gSpecular;
layout(location = 3) out vec3 gEmissive;

in vec3 transformedNormal;

layout(location = 2)
uniform vec3 diffuseColor;
layout(location = 3)
uniform vec3 specularColor;
layout(location = 4)
uniform vec3 emissiveColor;
layout(location = 5)
uniform float shininess;

void main()
{
    // store albedo information
    gAlbedo = diffuseColor;
    // store specular intensity
    gSpecular = vec4(specularColor, max(shininess, 0.01f));
    // store per fragment normal
    gNormal = normalize(transformedNormal);
    // per fragment emissivenes
    gEmissive = emissiveColor;
}