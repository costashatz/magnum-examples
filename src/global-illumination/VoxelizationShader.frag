/* inputs from geometry shader */
#if defined(AMBIENT_TEXTURE) || defined(DIFFUSE_TEXTURE) || defined(SPECULAR_TEXTURE)
in vec2 texCoords;
#endif
in vec3 worldNormal;
in vec3 worldPosition;
in flat int dominantAxis;
in flat int voxelDims;
in vec4 boundingBox;
in vec3 texPos;

/* uniforms */
/* the voxel texture to create the octree */
uniform layout(binding = 0, RGBA8) image3D voxelTexture;
/* material textures */
#ifdef AMBIENT_TEXTURE
uniform layout(binding = 1) sampler2D ambientTexture;
#endif

#ifdef DIFFUSE_TEXTURE
uniform layout(binding = 2) sampler2D diffuseTexture;
#endif

layout(location = 7)
uniform vec4 ambientColor
#ifndef AMBIENT_TEXTURE
    = vec4(0.0, 0.0, 0.0, 1.0);
#else
    = vec4(1.0);
#endif

layout(location = 8)
uniform vec4 diffuseColor = vec4(1.);

/* @todo: put this in common file to be included */
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

vec4 directLight(Light light, float visibility, vec4 finalDiffuseColor) {
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

    return attenuation * diffuseReflection;
}

void main() {
	/* checks for conservative rasterization */
	if(gl_FragCoord.x < boundingBox.x || gl_FragCoord.y < boundingBox.y || gl_FragCoord.x > boundingBox.z || gl_FragCoord.y > boundingBox.w)
	   discard ;
	/* if something is outside the voxel bounding box
	   discard it
	 */
	if(texPos.x < 0. || texPos.x > 1. || texPos.y < 0. || texPos.y > 1. || texPos.z < 0. || texPos.z > 1.)
		discard;
	/* get ambient color either from texture or color */
    vec4 ambientColor =
    #ifdef AMBIENT_TEXTURE
        texture(ambientTexture, texCoords) *
    #endif
        ambientColor;
    /* get material color either from texture or color */
    vec4 finalDiffuseColor =
    #ifdef DIFFUSE_TEXTURE
        texture(diffuseTexture, texCoords) *
    #endif
        diffuseColor;

    /* @todo: create shadow map? */
    /* @todo: add transparency feature? */
    float visibility = 1.;

	/* get lighting information */
	vec4 materialColor = ambientColor + directLight(light, visibility, finalDiffuseColor);

	ivec3 texturePos = ivec3(voxelDims * texPos);

	/* Overwrite currently stored value.
     * @todo: Atomic operations to get an averaged value, described in OpenGL insights about voxelization
              Required to avoid flickering when voxelizing every frame */
    imageStore(voxelTexture, texturePos, vec4(materialColor.rgb * visibility, 1.));
}