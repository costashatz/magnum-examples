#define M_PI 3.1415926535897932384626433832795
#define M_PI_2 1.57079632679489661923
#define SQRT_3 1.73205080;
#define EPSILON 1e-30

struct lightSource
{
    vec4 position;
    vec4 diffuse;
    vec3 spotDirection;
    float spotExponent;
    float spotCutoff;
    float intensity;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

vec3 VoxelToWorld(ivec3 pos, float voxelSize, vec3 worldMinPoint)
{
    vec3 result = vec3(pos);
    result *= voxelSize;

    return result + worldMinPoint;
}

vec3 WorldToVoxel(vec3 position, float voxelScale, vec3 worldMinPoint)
{
    vec3 voxelPos = position - worldMinPoint;
    return voxelPos * voxelScale;
}

vec3 EncodeNormal(vec3 normal)
{
    return normal * 0.5 + vec3(0.5);
}

vec3 DecodeNormal(vec3 normal)
{
    return normal * 2. - vec3(1.);
}

void getLightValues(lightSource light, vec3 position, out vec3 lightDirection, out float attenuation)
{
    /* Directional light */
    if(light.position.w == 0.) {
        attenuation = 1.;
        lightDirection = normalize(-vec3(light.position));
    }
    /* Pointlight or Spotlight */
    else {
        lightDirection = vec3(light.position) - position;

        float distance = length(lightDirection);

        attenuation = light.intensity / (light.constantAttenuation +
                                                distance * (light.linearAttenuation + distance * light.quadraticAttenuation));

        lightDirection = normalize(lightDirection);

        /* Spotlight */
        if(attenuation > 0. && light.spotCutoff < M_PI / 2.) {
            float clampedCosine = max(0., dot(-lightDirection, light.spotDirection));
            /* Outsise of spotlight cone? */
            if(clampedCosine < cos(light.spotCutoff)) {
                attenuation = 0.;
            }
            else {
                attenuation = attenuation * pow(clampedCosine, light.spotExponent);
            }
        }
    }
}
