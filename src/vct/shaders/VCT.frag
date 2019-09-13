// #version 430

#ifndef LIGHT_COUNT
#define LIGHT_COUNT 1 // define this when compiling the shader
#endif

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

// TODO: Make these uniforms?
// Ambient Occlusion alpha
const float aoAlpha = 0.;
// AO falloff
const float aoFalloff = 800.;
// Maximum tracing distance
const float maxTracingDistanceGlobal = 0.95;
const float samplingFactor = 0.03;
const float bounceStrength = 1.;
const float coneShadowTolerance = 0.1;
const float coneShadowAperture = 0.2;

in vec2 textureCoordinates;

out vec4 color;

const vec3 diffuseConeDirections[] =
{
    vec3(0., 1., 0.),
    vec3(0., 0.5, 0.866025),
    vec3(0.823639, 0.5, 0.267617),
    vec3(0.509037, 0.5, -0.7006629),
    vec3(-0.509037, 0.5, -0.7006629),
    vec3(-0.823639, 0.5, 0.267617)
};

const float diffuseConeWeights[] =
{
    M_PI / 4.,
    3. * M_PI / 20.,
    3. * M_PI / 20.,
    3. * M_PI / 20.,
    3. * M_PI / 20.,
    3. * M_PI / 20.
};

vec3 PositionFromDepth()
{
    float z = texture(gDepth, textureCoordinates).x * 2. - 1.;
    vec4 projected = vec4(textureCoordinates * 2. - 1., z, 1.);
    projected = inverseProjectionView * projected;
    return projected.xyz / projected.w;
}

vec4 AnisotropicSample(vec3 coord, vec3 weight, uvec3 face, float lod)
{
    // anisotropic volumes level
    float anisoLevel = max(lod - 1., 0.);
    // directional sample
    vec4 anisoSample = weight.x * textureLod(voxelTexMipmap[face.x], coord, anisoLevel)
                     + weight.y * textureLod(voxelTexMipmap[face.y], coord, anisoLevel)
                     + weight.z * textureLod(voxelTexMipmap[face.z], coord, anisoLevel);
    // linearly interpolate on base level
    if(lod < 1.)
    {
        vec4 baseColor = texture(voxelTex, coord);
        anisoSample = mix(baseColor, anisoSample, clamp(lod, 0., 1.));
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
    enter = max(max(v3Min.x, 0.), max(v3Min.y, v3Min.z));

    return leave > enter;
}

vec4 TraceCone(vec3 position, vec3 normal, vec3 direction, float aperture, bool traceOcclusion)
{
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.) ? 0 : 1;
    visibleFace.y = (direction.y < 0.) ? 2 : 3;
    visibleFace.z = (direction.z < 0.) ? 4 : 5;
    traceOcclusion = traceOcclusion && aoAlpha < 1.;
    // world space grid voxel size
    float voxelWorldSize = 2.0 /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + normal * dst;
    // final results
    vec4 coneSample = vec4(0.);
    float occlusion = 0.;
    float maxDistance = maxTracingDistanceGlobal * (1. / voxelScale);
    float falloff = 0.5 * aoFalloff * voxelScale;
    // out of boundaries check
    float enter = 0.;
    float leave = 0.;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        coneSample.a = 1.;
    }

    while(coneSample.a < 1. && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        // cone expansion and respective mip level based on diameter
        float diameter = 2. * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition, voxelScale, worldMinPoint);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnisotropicSample(coord, weight, visibleFace, mipLevel);
        // front to back composition
        // coneSample += (1. - coneSample.a) * anisoSample;
        coneSample += pow(1. - coneSample.a, 2.) * anisoSample * exp(-0.8 * dst * dst / samplingFactor);
        // ambient occlusion
        if(traceOcclusion && occlusion < 1.)
        {
            occlusion += ((1. - occlusion) * anisoSample.a) / (1. + falloff * diameter);
        }
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return vec4(coneSample.rgb, occlusion);
}

float TraceShadowCone(vec3 position, vec3 direction, float aperture, float maxTracingDistance)
{
    bool hardShadows = false;

    if(coneShadowTolerance == 1.) { hardShadows = true; }

    // directional dominat axis
    uvec3 visibleFace;
    visibleFace.x = (direction.x < 0.) ? 0 : 1;
    visibleFace.y = (direction.y < 0.) ? 2 : 3;
    visibleFace.z = (direction.z < 0.) ? 4 : 5;
    // world space grid size
    float voxelWorldSize = 3. /  (voxelScale * volumeDimension);
    // weight per axis for aniso sampling
    vec3 weight = direction * direction;
    // move further to avoid self collision
    float dst = voxelWorldSize;
    vec3 startPosition = position + direction * dst;
    // control vars
    float mipMaxLevel = log2(volumeDimension) - 1.;
    // final results
    float visibility = 0.;
    float k = exp2(7. * coneShadowTolerance);
    // cone will only trace the needed distance
    float maxDistance = maxTracingDistance;
    // out of boundaries check
    float enter = 0.;
    float leave = 0.;

    if(!IntersectRayWithWorldAABB(position, direction, enter, leave))
    {
        visibility = 1.;
    }

    while(visibility < 1. && dst <= maxDistance)
    {
        vec3 conePosition = startPosition + direction * dst;
        float diameter = 2. * aperture * dst;
        float mipLevel = log2(diameter / voxelWorldSize);
        // convert position to texture coord
        vec3 coord = WorldToVoxel(conePosition, voxelScale, worldMinPoint);
        // get directional sample from anisotropic representation
        vec4 anisoSample = AnisotropicSample(coord, weight, visibleFace, mipLevel);

        // hard shadows exit as soon cone hits something
        if(hardShadows && anisoSample.a > EPSILON) { return 0.; }
        // accumulate
        visibility += (1. - visibility) * anisoSample.a * k;
        // move further into volume
        dst += diameter * samplingFactor;
    }

    return 1.0f - visibility;
}

vec4 CalculateIndirectLighting(vec3 position, vec3 normal, vec3 albedo, vec4 specular)
{
    vec4 specularTrace = vec4(0.);
    vec4 diffuseTrace = vec4(0.);
    vec3 coneDirection = vec3(0.);

    // component greater than zero
    if(any(greaterThan(specular.rgb, specularTrace.rgb)))
    {
        vec3 viewDirection = normalize(cameraPosition - position);
        vec3 coneDirection = reflect(-viewDirection, normal);
        coneDirection = normalize(coneDirection);
        // specular cone setup, minimum of 1 grad, fewer can severly slow down performance
        float aperture = clamp(tan(M_PI_2 * (1. - specular.a)), 0.0174533, M_PI);
        specularTrace = TraceCone(position, normal, coneDirection, aperture, false);
        specularTrace.rgb *= specular.rgb;
    }

    // component greater than zero
    if(any(greaterThan(albedo, diffuseTrace.rgb)))
    {
        // diffuse cone setup
        const float aperture = 0.57735; //0.325;
        vec3 guide = vec3(0., 1., 0.);

        if (abs(dot(normal, guide)) == 1.)
        {
            guide = vec3(0., 0., 1.);
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

        // TO-DO: Check how to do this blending correctly
        diffuseTrace.rgb *= (albedo + vec3(0.3f));
    }

    vec3 result = bounceStrength * (diffuseTrace.rgb + specularTrace.rgb);

    return vec4(result, clamp(1. - diffuseTrace.a + aoAlpha, 0., 1.));
}

// albedo should contain: material*lightDiffuse
vec3 BRDF(vec3 lightDirection, vec3 normal, vec3 albedo)
{
    // TO-DO: Make this better
    float intensity = dot(normal, lightDirection);

    /* Diffuse color */
    return albedo * max(0., intensity);
}

vec3 CalculateDirectLighting(vec3 position, vec3 normal, vec3 albedo)
{
    vec3 normalizedNormal = normalize(normal);
    // calculate directional lighting
    vec3 directLighting = vec3(0.);
    vec3 current =  vec3(0.);
    int count = 0;

    for(int i = 0; i != LIGHT_COUNT; ++i) {
        vec3 lightDirection;
        float attenuation;
        float visibility = 1.;

        float shadowMaxDist = 1.;

        getLightValues(lights[i], position, lightDirection, attenuation);

        if(attenuation <= 0.)
            current = vec3(0.);
        else {
            // vec3 coord = WorldToVoxel(position, voxelScale, worldMinPoint);
            // visibility = max(0.0f, texture(voxelVisibility, coord).a);
            visibility = TraceShadowCone(position, lightDirection, coneShadowAperture, shadowMaxDist);
            if(visibility <= 0.)
                current = vec3(0.);
            else
                current = BRDF(lightDirection, normalizedNormal, visibility * attenuation * lights[i].diffuse.rgb * albedo);
        }


        directLighting += current;
    }

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
    vec3 albedo = pow(baseColor, vec3(2.2));
    // fragment emissiviness
    vec3 emissive = texture(gEmissive, textureCoordinates).rgb;

    // lighting cumulatives
    vec3 directLighting = vec3(0.);
    // rgb, a = ambient occlusion
    vec4 indirectLighting = vec4(1.);

    indirectLighting = CalculateIndirectLighting(position, normal, baseColor, specular);
    directLighting = CalculateDirectLighting(position, normal, albedo);

    // convert indirect to linear space
    indirectLighting.rgb = pow(indirectLighting.rgb, vec3(2.2));
    // final composite lighting (direct + indirect) * ambient occlusion
    vec3 compositeLighting = (directLighting + indirectLighting.rgb) * indirectLighting.a;
    compositeLighting += emissive;
    // compositeLighting.rgb = vec3(1. - indirectLighting.a);
    // -- this could be done in a post-process pass --

    // Reinhard tone mapping
    compositeLighting = compositeLighting / (compositeLighting + 1.);
    // gamma correction
    const float gamma = 2.2;
    // convert to gamma space
    compositeLighting = pow(compositeLighting, vec3(1. / gamma));

    color = vec4(compositeLighting, 1.);
}