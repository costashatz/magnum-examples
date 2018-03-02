/* inputs from geometry shader */
#ifdef DIFFUSE_TEXTURE
in vec2 texCoords;
#endif
in vec3 normal;
in flat int dominantAxis;
in flat int voxelDims;
in vec4 boundingBox;
in vec3 vertexPos;

/* uniforms */
/* the voxel texture to create the octree */
uniform layout(binding = 0, RGBA8) image3D voxelTexture;
/* diffuse texture only */
#ifdef DIFFUSE_TEXTURE
uniform layout(binding = 1) sampler2D diffuseTexture;
#endif

layout(location = 4)
uniform vec4 diffuseColor = vec4(1.);

layout (location = 0) out vec4 gl_FragColor;
layout (pixel_center_integer) in vec4 gl_FragCoord;

void main() {
	// /* checks for conservative rasterization */
	// if(vertexPos.x < boundingBox.x || vertexPos.y < boundingBox.y || vertexPos.x > boundingBox.z || vertexPos.y > boundingBox.w)
	//    discard ;
    /* get material color either from texture or color */
    vec4 materialColor = 
    #ifdef DIFFUSE_TEXTURE
        texture(diffuseTexture, texCoords) *
    #endif
        diffuseColor;

    /* @todo: maybe also include direct diffuse lighting things? */

    /* @todo: create shadow map? */
    /* @todo: add transparency feature? */
    float visibility = 1.;

	ivec3 camPos = ivec3(gl_FragCoord.x, gl_FragCoord.y, voxelDims * gl_FragCoord.z);
	ivec3 texPos;
	if(dominantAxis == 1) {
	    texPos.x = voxelDims - camPos.z;
		texPos.z = camPos.x;
		texPos.y = camPos.y;
	}
	else if(dominantAxis == 2) {
	    texPos.z = camPos.y;
		texPos.y = voxelDims - camPos.z;
		texPos.x = camPos.x;
	} else {
	    texPos = camPos;
	}

	/* flipt z coordinate */
	texPos.z = voxelDims - texPos.z - 1;

	/* Overwrite currently stored value.
     * @todo: Atomic operations to get an averaged value, described in OpenGL insights about voxelization
              Required to avoid flickering when voxelizing every frame */
    imageStore(voxelTexture, texPos, vec4(materialColor.rgb * visibility, 1.));
}