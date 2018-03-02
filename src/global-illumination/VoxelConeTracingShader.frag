/* inputs from vertex shader */
#if defined(AMBIENT_TEXTURE) || defined(DIFFUSE_TEXTURE) || defined(SPECULAR_TEXTURE)
in vec2 textureCoords;
#endif
in vec3 worldPosition;
in vec3 worldNormal;
in vec3 eyeDir;
in mat3 normalMatrix;

/* uniforms */
/* voxel world */
uniform layout(binding = 0) sampler3D voxelTexture;

layout(location = 3)
uniform float voxelGridWorldSize;

layout(location = 4)
uniform int voxelDimensions;

/* material textures */
#ifdef AMBIENT_TEXTURE
uniform layout(binding = 1) sampler2D ambientTexture;
#endif

#ifdef DIFFUSE_TEXTURE
uniform layout(binding = 2) sampler2D diffuseTexture;
#endif

#ifdef SPECULAR_TEXTURE
uniform layout(binding = 3) sampler2D specularTexture;
#endif

/* material colors/properties */
layout(location = 5)
uniform vec4 ambientColor
#ifndef AMBIENT_TEXTURE
    = vec4(0.0, 0.0, 0.0, 1.0);
#else
    = vec4(1.0);
#endif

layout(location = 6)
uniform vec4 diffuseColor = vec4(1.);

layout(location = 7)
uniform vec4 specularColor = vec4(1.);

layout(location = 8)
uniform vec4 emissiveColor = vec4(0.0, 0.0, 0.0, 1.0);

layout(location = 9)
uniform float shininess = 80.;

/* lights */
struct Light {
    int type; /* 0 - infinite/directional, 1 - point, 2 - spot */
    vec3 color;
    vec3 position;
    vec3 direction;
    float spotExponent;
    float spotCutoff;
    float intensity;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

/* @todo: use multiple lights with shader buffers */
layout(location = 10)
uniform Light light;

/* output color */
out vec4 color;

/* global variables/defines */
const float maximumDistance = 100.0;
const float alphaThreshold = 0.95;

// 6 60 degree cone
const int numberOfCones = 6;
vec3 coneDirections[6] = vec3[]
(                            vec3(0, 1, 0),
                            vec3(0, 0.5, 0.866025),
                            vec3(0.823639, 0.5, 0.267617),
                            vec3(0.509037, 0.5, -0.700629),
                            vec3(-0.509037, 0.5, -0.700629),
                            vec3(-0.823639, 0.5, 0.267617)
                            );
float coneWeights[6] = float[](0.25, 0.15, 0.15, 0.15, 0.15, 0.15);

// // 5 90 degree cones
// const int numberOfCones = 5;
// vec3 coneDirections[5] = vec3[]
// (                            vec3(0, 1, 0),
//                             vec3(0, 0.707, 0.707),
//                             vec3(0, 0.707, -0.707),
//                             vec3(0.707, 0.707, 0),
//                             vec3(-0.707, 0.707, 0)
//                             );
// float coneWeights[5] = float[](0.28, 0.18, 0.18, 0.18, 0.18);

/* helper function to get a vector orthogonal to another*/
/* assumes v is already normalized */
vec3 orthogonal(vec3 u){
    /* Pick any normalized vector. */
	vec3 v = vec3(0.99146, 0.11664, 0.05832);
	return abs(dot(u, v)) > 0.99999 ? cross(u, vec3(0, 1, 0)) : cross(u, v);
}

vec4 getVoxel(vec3 position, float lod) {
    vec3 offset = vec3(1. / voxelDimensions, 1. / voxelDimensions, 0.);
    /* position.xyz can take values (-voxelGridWorldSize/2., voxelGridWorldSize) */
    vec3 voxelCoordinates = (position / voxelGridWorldSize) + 0.5 + offset;
    return textureLod(voxelTexture, voxelCoordinates, lod);
}


/* trace cone in voxel grid */
vec4 coneTrace(vec3 pos, vec3 direction, float tanHalfAngle, float maxDistance, out float occlusion) {
    
    /* lod level 0 mipmap is full size,
     * level 1 is half that size and so on
     */
    float lod = 0.;
    vec3 color = vec3(0.);
    float alpha = 0.;
    occlusion = 0.;

    float voxelWorldSize = voxelGridWorldSize / voxelDimensions;
    float dist = voxelWorldSize; /* Start one voxel away to avoid self occlusion */
    vec3 startPos = pos + worldNormal * voxelWorldSize; /* Plus move away slightly in the normal direction to avoid
                                                                    self occlusion in flat surfaces */

    while(dist < maxDistance && alpha < alphaThreshold) {
        /* smallest sample diameter possible is the voxel size */
        float diameter = max(voxelWorldSize, 2.0 * tanHalfAngle * dist);
        float lodLevel = log2(diameter / voxelWorldSize);
        vec4 voxelColor = getVoxel(startPos + dist * direction, lodLevel);

        /* front-to-back compositing */
        float a = (1. - alpha);
        color += a * voxelColor.rgb;
        alpha += a * voxelColor.a;
        occlusion += (a * voxelColor.a) / (1.0 + 0.03 * diameter);
        dist += diameter * 0.5; /* smoother */
        /* dist += diameter; /* faster but misses more voxels */
    }

    return vec4(color, alpha);
}

/* trace indirect diffuse light */
vec4 indirectDiffuseLight(out float occlusion_out) {
    vec4 color = vec4(0.);
    occlusion_out = 0.;

    for(int i = 0; i < numberOfCones; i++) {
        float occlusion = 0.;
        /* 60 degree cones -> tan(30) = 0.577
           90 degree cones -> tan(45) = 1.0 */
        color += coneWeights[i] * coneTrace(worldPosition, normalize(normalMatrix * coneDirections[i]), 0.577, maximumDistance, occlusion);
        occlusion_out += coneWeights[i] * occlusion;
    }

    // const float ANGLE_MIX = 0.5f; // Angle mix (1.0f => orthogonal direction, 0.0f => direction of normal).

	// const float w[3] = {1.0, 1.0, 1.0}; // Cone weights.

    // float voxelWorldSize = voxelGridWorldSize / voxelDimensions;
    // vec3 origin = worldPosition + worldNormal * voxelWorldSize;
    // float coneOffset = -0.001;

	// // Find a base for the side cones with the normal as one of its base vectors.
	// const vec3 ortho = normalize(orthogonal(worldNormal));
	// const vec3 ortho2 = normalize(cross(ortho, worldNormal));

	// // Find base vectors for the corner cones too.
	// const vec3 corner = 0.5f * (ortho + ortho2);
	// const vec3 corner2 = 0.5f * (ortho - ortho2);

	// // Trace front cone
    // float occlusion = 0.;
	// color += w[0] * coneTrace(origin + coneOffset * worldNormal, worldNormal, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[0] * occlusion;

	// // Trace 4 side cones.
	// const vec3 s1 = mix(worldNormal, ortho, ANGLE_MIX);
	// const vec3 s2 = mix(worldNormal, -ortho, ANGLE_MIX);
	// const vec3 s3 = mix(worldNormal, ortho2, ANGLE_MIX);
	// const vec3 s4 = mix(worldNormal, -ortho2, ANGLE_MIX);

    // occlusion = 0.;
    // color += w[1] * coneTrace(origin + coneOffset * ortho, s1, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[1] * occlusion;

    // occlusion = 0.;
    // color += w[1] * coneTrace(origin - coneOffset * ortho, s2, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[1] * occlusion;

    // occlusion = 0.;
    // color += w[1] * coneTrace(origin + coneOffset * ortho2, s3, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[1] * occlusion;

    // occlusion = 0.;
	// color += w[1] * coneTrace(origin - coneOffset * ortho2, s4, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[1] * occlusion;

	// // Trace 4 corner cones.
	// const vec3 c1 = mix(worldNormal, corner, ANGLE_MIX);
	// const vec3 c2 = mix(worldNormal, -corner, ANGLE_MIX);
	// const vec3 c3 = mix(worldNormal, corner2, ANGLE_MIX);
	// const vec3 c4 = mix(worldNormal, -corner2, ANGLE_MIX);

	// occlusion = 0.;
    // color += w[2] * coneTrace(origin + coneOffset * corner, c1, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[2] * occlusion;

    // occlusion = 0.;
    // color += w[2] * coneTrace(origin - coneOffset * corner, c2, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[2] * occlusion;

    // occlusion = 0.;
    // color += w[2] * coneTrace(origin + coneOffset * corner2, c3, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[2] * occlusion;

    // occlusion = 0.;
	// color += w[2] * coneTrace(origin - coneOffset * corner2, c4, 0.577, maximumDistance, occlusion);
    // occlusion_out += w[2] * occlusion;

    occlusion_out = 1. - occlusion_out;

    return color;
}

vec4 directLight(Light light, float visibility, vec4 finalDiffuseColor, vec4 finalSpecularColor, vec3 normalizedEyeDir) {
    /* if the object is not visible, return */
    if(visibility == 0.)
        return vec4(0., 0., 0., 1.);
    vec3 lightDirection;
    float attenuation;

    /* calculate attenuation and light direction */
    if(light.type == 0) {
        lightDirection = light.direction;
        attenuation = light.intensity;
    }
    else {
        lightDirection = light.position - worldPosition;

        float distance = length(lightDirection);

        attenuation = light.intensity / (light.constantAttenuation + 
                                                distance * (light.linearAttenuation + distance * light.quadraticAttenuation));

        lightDirection = normalize(lightDirection);

        /* spot-light? */
        if(light.type == 2) {
            float clampedCosine = max(0.0, dot(-lightDirection, light.direction));
            /* Outsise of spotlight cone? */
            if(clampedCosine < cos(light.spotCutoff)) {
                attenuation = 0.0;
            }
            else {
                attenuation = attenuation * pow(clampedCosine, light.spotExponent);
            }
        }
    }

    float lightAngle = dot(worldNormal, lightDirection);

    /* Diffuse color */
    float diffuseAngle = visibility * max(0.0, lightAngle);
    vec4 diffuseReflection = vec4(light.color, 1.) * finalDiffuseColor * diffuseAngle;  

    /* Specular color */
    vec4 specularReflection = vec4(0., 0., 0., 1.);
    if(lightAngle >= 0.0) {
        vec3 reflection = reflect(-lightDirection, worldNormal);
        float specularity = visibility * pow(max(0.0, dot(normalizedEyeDir, reflection)), shininess);
        specularReflection = vec4(light.color, 1.) * finalSpecularColor * specularity;
    }

    return attenuation * (diffuseReflection + specularReflection);
}

void main() {
    /* get ambient color either from texture or color */
    vec4 finalAmbientColor = 
    #ifdef AMBIENT_TEXTURE
        texture(ambientTexture, texCoords) *
    #endif
        ambientColor;
    /* get diffuse color either from texture or color */
    vec4 finalDiffuseColor = 
    #ifdef DIFFUSE_TEXTURE
        texture(diffuseTexture, texCoords) *
    #endif
        diffuseColor;
    /* get specular color either from texture or color */
    vec4 finalSpecularColor = 
    #ifdef SPECULAR_TEXTURE
        texture(specularTexture, texCoords) *
    #endif
        specularColor;
    /* get emissive color */
    vec4 finalEmissiveColor = emissiveColor;

    /* eye direction */
    vec3 normalizedEyeDir = normalize(eyeDir);

    /* @todo: create shadow map? */
    /* @todo: add transparency feature? */
    float visibility = 1.;

    /* add direct lighting */
    vec4 directLighting = directLight(light, visibility, finalDiffuseColor, finalSpecularColor, normalizedEyeDir);

    /* @todo: compute indirect diffuse */
    float occlusion;
    vec4 diffuseIndirect = indirectDiffuseLight(occlusion) * finalDiffuseColor;
    // occlusion = min(1.0, 1.5 * occlusion);
    // diffuseIndirect = 2. * occlusion * diffuseIndirect * finalDiffuseColor;

    /* @todo: compute specular 2nd bounce */
    vec4 specularBounce = vec4(0., 0., 0., 1.);

    /* @todo: compute indirect refractive if transparent object */
    vec4 refractiveIndirect = vec4(0., 0., 0., 1.);
    
    vec4 finalColor = directLighting + diffuseIndirect + specularBounce + refractiveIndirect;

	color = finalAmbientColor + finalEmissiveColor + finalColor;
}