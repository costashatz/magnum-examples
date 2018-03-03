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
const float maximumDistance = distance(vec3(abs(worldPosition)), vec3(-voxelGridWorldSize / 2.));;
const float alphaThreshold = 0.95;

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
vec4 coneTrace(vec3 pos, vec3 direction, float tanHalfAngle, float maxDistance) {
    /* lod level 0 mipmap is full size,
     * level 1 is half that size and so on
     */
    float lod = 0.;
    vec3 color = vec3(0.);
    float alpha = 0.;

    float voxelWorldSize = voxelGridWorldSize / voxelDimensions;
    float dist = voxelWorldSize; /* Start one voxel away to avoid self occlusion */
    vec3 startPos = pos;

    while(dist < maxDistance && alpha < alphaThreshold) {
        /* smallest sample diameter possible is the voxel size */
        float diameter = max(voxelWorldSize, 2.0 * tanHalfAngle * dist);
        float lodLevel = log2(diameter / voxelWorldSize);
        vec4 voxelColor = getVoxel(startPos + dist * direction, lodLevel);

        /* front-to-back compositing */
        float a = (1. - alpha);
        color += a * voxelColor.rgb;
        alpha += a * voxelColor.a;
        dist += diameter * 0.5; /* smoother */
        /* dist += diameter; /* faster but misses more voxels */
    }

    return vec4(color, alpha);
}

/* trace indirect diffuse light */
vec4 indirectDiffuseLight() {
    vec4 color = vec4(0.);

    /* Angle mix (1.0f => orthogonal direction, 0.0f => direction of normal) */
    const float angleMix = 0.5f;

    /* Cone weights. */
    // const float w[3] = {0.25, 0.25, 0.25};
    const float w[3] = {0.15, 0.15, 0.15};

    /* Starting position
     * We offset forward in normal direction, and backward in cone direction.
     * Backward in cone direction improves GI, and forward direction removes
     * artifacts
     */
    float voxelWorldSize = voxelGridWorldSize / voxelDimensions;
    vec3 origin = worldPosition + worldNormal * (1. + 2. * 0.707106) * voxelWorldSize;
    float coneOffset = -0.001;

	/* Find a base for the side cones with the normal as one of its base vectors */
	const vec3 ortho = normalize(orthogonal(worldNormal));
	const vec3 ortho2 = normalize(cross(ortho, worldNormal));

	/* Find base vectors for the corner cones too */
	const vec3 corner = 0.5f * (ortho + ortho2);
	const vec3 corner2 = 0.5f * (ortho - ortho2);

	/* Trace front cone */
	color += w[0] * coneTrace(origin + coneOffset * worldNormal, worldNormal, 0.325, maximumDistance);

	/* Trace 4 side cones */
	const vec3 s1 = mix(worldNormal, ortho, angleMix);
	const vec3 s2 = mix(worldNormal, -ortho, angleMix);
	const vec3 s3 = mix(worldNormal, ortho2, angleMix);
	const vec3 s4 = mix(worldNormal, -ortho2, angleMix);

    color += w[1] * coneTrace(origin + coneOffset * ortho, s1, 0.325, maximumDistance);
    color += w[1] * coneTrace(origin - coneOffset * ortho, s2, 0.325, maximumDistance);
    color += w[1] * coneTrace(origin + coneOffset * ortho2, s3, 0.325, maximumDistance);
	color += w[1] * coneTrace(origin - coneOffset * ortho2, s4, 0.325, maximumDistance);

	/* Trace 4 corner cones */
	const vec3 c1 = mix(worldNormal, corner, angleMix);
	const vec3 c2 = mix(worldNormal, -corner, angleMix);
	const vec3 c3 = mix(worldNormal, corner2, angleMix);
	const vec3 c4 = mix(worldNormal, -corner2, angleMix);

    color += w[2] * coneTrace(origin + coneOffset * corner, c1, 0.325, maximumDistance);
    color += w[2] * coneTrace(origin - coneOffset * corner, c2, 0.325, maximumDistance);
    color += w[2] * coneTrace(origin + coneOffset * corner2, c3, 0.325, maximumDistance);
	color += w[2] * coneTrace(origin - coneOffset * corner2, c4, 0.325, maximumDistance);

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
    vec4 diffuseIndirect = indirectDiffuseLight() * finalDiffuseColor;

    /* @todo: compute specular 2nd bounce */
    float voxelWorldSize = voxelGridWorldSize / voxelDimensions;
    vec3 reflection = normalize(reflect(normalizedEyeDir, worldNormal));
    vec3 origin = worldPosition + worldNormal * (1. + 8. * 0.707106) * voxelWorldSize;
    vec4 specularBounce = 0.5 * coneTrace(origin, reflection, 0.577, maximumDistance);

    /* @todo: compute indirect refractive if transparent object */
    vec4 refractiveIndirect = vec4(0., 0., 0., 1.);
    
    vec4 finalColor = directLighting + diffuseIndirect;// + specularBounce + refractiveIndirect;

	color = finalAmbientColor + finalEmissiveColor + finalColor;
}