// #version 430

#ifndef LIGHT_COUNT
#define LIGHT_COUNT 1 // define this when compiling the shader
#endif

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

layout(binding = 0)
uniform sampler2D gAlbedo;
layout(binding = 1)
uniform sampler2D gNormal;
layout(binding = 2)
uniform sampler2D gSpecular;
layout(binding = 3)
uniform sampler2D gEmissive;
layout(binding = 4)
uniform sampler2D gDepth;

layout(binding = 5)
uniform sampler3D voxelVisibility;
layout(binding = 6)
uniform sampler3D voxelTex;
layout(binding = 7)
uniform sampler3D voxelTexMipmap[6];

layout(location = 0)
uniform mat4 inverseProjectionView;
layout(location = 1)
uniform vec3 cameraPosition;
layout(location = 2)
uniform float voxelScale;
layout(location = 3)
uniform vec3 worldMinPoint;
layout(location = 4)
uniform vec3 worldMaxPoint;
layout(location = 5)
uniform uint volumeDimension;

layout(location = 6)
uniform lightSource lights[LIGHT_COUNT];

const float PI = 3.14159265f;
const float HALF_PI = 1.57079f;
const float EPSILON = 1e-30;
const float SQRT_3 = 1.73205080f;

// TODO: Make these uniforms?
// Ambient Occlusion alpha
const float aoAlpha = 0.f;
// AO falloff
const float aoFalloff = 800.0f;
// Maximum tracing distance
const float maxTracingDistanceGlobal = 0.95f;
const float samplingFactor = 0.5f;
const float bounceStrength = 2.f;
const float coneShadowTolerance = 0.1f;
const float coneShadowAperture = 0.2f;

in vec2 textureCoordinates;

out vec4 color;

const vec3 diffuseConeDirections[] =
{
    vec3(0.f, 1.f, 0.f),
    vec3(0.f, 0.5f, 0.866025f),
    vec3(0.823639f, 0.5f, 0.267617f),
    vec3(0.509037f, 0.5f, -0.7006629f),
    vec3(-0.509037f, 0.5f, -0.7006629f),
    vec3(-0.823639f, 0.5f, 0.267617f)
};

const float diffuseConeWeights[] =
{
    PI / 4.f,
    3.f * PI / 20.f,
    3.f * PI / 20.f,
    3.f * PI / 20.f,
    3.f * PI / 20.f,
    3.f * PI / 20.f
};

vec3 WorldToVoxel(vec3 position)
{
    vec3 voxelPos = position - worldMinPoint;
    return voxelPos * voxelScale;
}

vec3 PositionFromDepth()
{
    float z = texture(gDepth, textureCoordinates).x * 2.f - 1.f;
    vec4 projected = vec4(textureCoordinates * 2.f - 1.f, z, 1.f);
    projected = inverseProjectionView * projected;
    return projected.xyz / projected.w;
}

vec4 AnisotropicSample(vec3 coord, vec3 weight, uvec3 face, float lod)
{
    // anisotropic volumes level
    float anisoLevel = max(lod - 1.0f, 0.0f);
    // directional sample
    vec4 anisoSample = weight.x * textureLod(voxelTexMipmap[face.x], coord, anisoLevel)
                     + weight.y * textureLod(voxelTexMipmap[face.y], coord, anisoLevel)
                     + weight.z * textureLod(voxelTexMipmap[face.z], coord, anisoLevel);
    // linearly interpolate on base level
    if(lod < 1.0f)
    {
        vec4 baseColor = texture(voxelTex, coord);
        anisoSample = mix(baseColor, anisoSample, clamp(lod, 0.0f, 1.0f));
    }

    return anisoSample;
}

bool IntersectRayWithWorldAABB(vec3 ro, vec3 rd, out float enter, out float leave)
{
    vec3 tempMin = (worldMinPoint - ro) / rd;
    vec3 tempMax = (worldMaxPoint - ro) / rd;

    vec3 v3Max = max(tempMax, tempMin);
    vec3 v3Min = min(tempMax, tempMin);

    leave = min(v3Max.x, min(v3Max.y, v3Max.z));
    enter = max(max(v3Min.x, 0.0), max(v3Min.y, v3Min.z));

    return leave > enter;
}

vec4 TraceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.0) ? 0 : 1;
    visibleFace.y = (direction.y < 0.0) ? 2 : 3;
    visibleFace.z = (direction.z < 0.0) ? 4 : 5;
    traceOcclusion = traceOcclusion && aoAlpha < 1.0f;
    // world space grid voxel size
    float voxelWorldSize = 2.0 /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + normal * dst;
    // final results
    vec4 coneSample = vec4(0.0f);
    float occlusion = 0.0f;
    float maxDistance = maxTracingDistanceGlobal * (1.0f / voxelScale);
    float falloff = 0.5f * aoFalloff * voxelScale;
    // out of boundaries check
    float enter = 0.0;
    float leave = 0.0;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        coneSample.a = 1.0f;
    }

    while(coneSample.a < 1.0f && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        // cone expansion and respective mip level based on diameter
        float diameter = 2.0f * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnisotropicSample(coord, weight, visibleFace, mipLevel);
        // front to back composition
        coneSample += (1.0f - coneSample.a) * anisoSample;
        // ambient occlusion
        if(traceOcclusion && occlusion < 1.0)
        {
            occlusion += ((1.0f - occlusion) * anisoSample.a) / (1.0f + falloff * diameter);
        }
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return vec4(coneSample.rgb, occlusion);
}

float TraceShadowCone(vec3 position, vec3 direction, float aperture, float maxTracingDistance) 
{
    bool hardShadows = false;

    if(coneShadowTolerance == 1.0f) { hardShadows = true; }

    // directional dominat axis
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.0) ? 0 : 1;
    visibleFace.y = (direction.y < 0.0) ? 2 : 3;
    visibleFace.z = (direction.z < 0.0) ? 4 : 5;
    // world space grid size
    float voxelWorldSize = 3.f /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + direction * dst;
    // control vars
    float mipMaxLevel = log2(volumeDimension) - 1.0f;
    // final results
    float visibility = 0.0f;
    float k = exp2(7.0f * coneShadowTolerance);
    // cone will only trace the needed distance
    float maxDistance = maxTracingDistance;
    // out of boundaries check
    float enter = 0.0; float leave = 0.0;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        visibility = 1.0f;
    }
    
    while(visibility < 1.0f && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        float diameter = 2.0f * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnisotropicSample(coord, weight, visibleFace, mipLevel);

        // hard shadows exit as soon cone hits something
        if(hardShadows && anisoSample.a > EPSILON) { return 0.0f; }  
        // accumulate
        visibility += (1.0f - visibility) * anisoSample.a * k;
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return 1.0f - visibility;
}

vec4 CalculateIndirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular)
{
    vec4 specularTrace = vec4(0.0f);
    vec4 diffuseTrace = vec4(0.0f);
    vec3 coneDirection = vec3(0.0f);

    // component greater than zero
    if(any(greaterThan(specular.rgb, specularTrace.rgb)))
    {
        vec3 viewDirection = normalize(cameraPosition - position);
        vec3 coneDirection = reflect(-viewDirection, normal);
        coneDirection = normalize(coneDirection);
        // specular cone setup, minimum of 1 grad, fewer can severly slow down performance
        float aperture = clamp(tan(HALF_PI * (1.0f - specular.a)), 0.0174533f, PI);
        specularTrace = TraceCone(position, normal, coneDirection, aperture, false);
        specularTrace.rgb *= specular.rgb;
    }

    // component greater than zero
    if(any(greaterThan(albedo, diffuseTrace.rgb)))
    {
        // diffuse cone setup
        const float aperture = 0.57735f;
        vec3 guide = vec3(0.0f, 1.0f, 0.0f);

        if (abs(dot(normal, guide)) == 1.0f)
        {
            guide = vec3(0.0f, 0.0f, 1.0f);
        }

        // Find a tangent and a bitangent
        vec3 right = normalize(guide - dot(normal, guide) * normal);
        vec3 up = cross(right, normal);

        for(int i = 0; i < 6; i++)
        {
            coneDirection = normal;
            coneDirection += diffuseConeDirections[i].x * right + diffuseConeDirections[i].z * up;
            coneDirection = normalize(coneDirection);
            // cumulative result
            diffuseTrace += TraceCone(position, normal, coneDirection, aperture, true) * diffuseConeWeights[i];
        }

        diffuseTrace.rgb *= albedo;
    }

    vec3 result = bounceStrength * (diffuseTrace.rgb + specularTrace.rgb);

    return vec4(result, clamp(1.0f - diffuseTrace.a + aoAlpha, 0.0f, 1.0f));
}

// albedo should contain: material*lightDiffuse
vec3 BRDF(vec3 lightDirection, vec3 normal, vec3 albedo)
{
    // TO-DO: Make this better
    float intensity = dot(normal, lightDirection);

    /* Diffuse color */
    return albedo * max(0.0, intensity);
}

vec4 CalculateDirectLighting(vec3 position, vec3 normal, vec3 albedo)
{
    vec3 normalizedNormal = normalize(normal);
    // calculate directional lighting
    vec4 directLighting = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    vec4 current =  vec4(0.0f);
    int count = 0;

    for(int i = 0; i != LIGHT_COUNT; ++i) {
        vec3 lightDirection;
        float attenuation;
        float visibility = 1.f;

        float shadowMaxDist = 1.f;

        /* Directional light */
        if(lights[i].position.w == 0.0) {
            attenuation = 1.0;
            lightDirection = normalize(-vec3(lights[i].position));
        }
        /* Pointlight or Spotlight */
        else {
            lightDirection = vec3(lights[i].position) - position;

            float dist = length(lightDirection);

            attenuation = lights[i].intensity / (lights[i].constantAttenuation +
                                                    dist * (lights[i].linearAttenuation + dist * lights[i].quadraticAttenuation));

            lightDirection = normalize(lightDirection);

            shadowMaxDist = dist;

            /* Spotlight */
            if(lights[i].spotCutoff < PI / 2.f) {
                float clampedCosine = max(0.0, dot(-lightDirection, lights[i].spotDirection));
                /* Outsise of spotlight cone? */
                if(clampedCosine < cos(lights[i].spotCutoff)) {
                    attenuation = 0.0;
                }
                else {
                    attenuation = attenuation * pow(clampedCosine, lights[i].spotExponent);
                }
            }
        }

        if(attenuation <= 0.f)
            current = vec4(0.f);
        else {
            // vec3 coord = WorldToVoxel(position);
            // visibility = max(0.0f, texture(voxelVisibility, coord).a);
            visibility = TraceShadowCone(position, lightDirection, coneShadowAperture, shadowMaxDist);
            if(visibility <= 0.f)
                current = vec4(0.f);
            else
                current = vec4(BRDF(lightDirection, normalizedNormal, visibility * attenuation * lights[i].diffuse.rgb * albedo), visibility);
        }


        directLighting.rgb += current.rgb;
        directLighting.a += current.a;
        count++;
    }

    if(count > 0) { directLighting.a /= count; }

    return directLighting;
}

void main()
{
    // world-space position
    vec3 position = PositionFromDepth();
    // world-space normal
    vec3 normal = normalize(texture(gNormal, textureCoordinates).xyz);
    // xyz = fragment specular, w = shininess
    vec4 specular = texture(gSpecular, textureCoordinates);
    // fragment albedo
    vec3 baseColor = texture(gAlbedo, textureCoordinates).rgb;
    // convert to linear space
    vec3 albedo = pow(baseColor, vec3(2.2f));
    // fragment emissiviness
    vec3 emissive = texture(gEmissive, textureCoordinates).rgb;

    // lighting cumulatives
    vec3 directLighting = vec3(0.0f);
    // rgb, a = ambient occlusion
    vec4 indirectLighting = vec4(1.0f);

    indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular);
    vec4 dirLight = CalculateDirectLighting(position, normal, albedo);
    directLighting = dirLight.rgb;

    // convert indirect to linear space
    indirectLighting.rgb = pow(indirectLighting.rgb, vec3(2.2f));
    // final composite lighting (direct + indirect) * ambient occlusion
    vec3 compositeLighting = (directLighting + indirectLighting.rgb) * indirectLighting.a;
    compositeLighting += emissive;
    // compositeLighting.rgb = vec3(1.f - indirectLighting.a);
    // -- this could be done in a post-process pass --

    // Reinhard tone mapping
    compositeLighting = compositeLighting / (compositeLighting + 1.0f);
    // gamma correction
    const float gamma = 2.2;
    // convert to gamma space
    compositeLighting = pow(compositeLighting, vec3(1.0 / gamma));

    color = vec4(compositeLighting, 1.0f);
}