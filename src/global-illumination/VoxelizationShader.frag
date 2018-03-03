/* inputs from geometry shader */
#ifdef DIFFUSE_TEXTURE
in vec2 texCoords;
#endif
in vec3 normal;
in flat int dominantAxis;
in flat int voxelDims;
in vec4 boundingBox;
in vec3 vertexPos;
in vec3 texPos;

/* uniforms */
/* the voxel texture to create the octree */
uniform layout(binding = 0, RGBA8) image3D voxelTexture;
/* diffuse texture only */
#ifdef DIFFUSE_TEXTURE
uniform layout(binding = 1) sampler2D diffuseTexture;
#endif

layout(location = 4)
uniform vec4 diffuseColor = vec4(1.);

void main() {
	// /* checks for conservative rasterization */
	// if(vertexPos.x < boundingBox.x || vertexPos.y < boundingBox.y || vertexPos.x > boundingBox.z || vertexPos.y > boundingBox.w)
	//    discard ;
	/* if something is outside the voxel bounding box
	   discard it
	 */
	if(texPos.x < 0. || texPos.x > 1. || texPos.y < 0. || texPos.y > 1. || texPos.z < 0. || texPos.z > 1.)
		discard;
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
	ivec3 texturePos = ivec3(voxelDims * texPos);

	/* Overwrite currently stored value.
     * @todo: Atomic operations to get an averaged value, described in OpenGL insights about voxelization
              Required to avoid flickering when voxelizing every frame */
    imageStore(voxelTexture, texturePos, vec4(materialColor.rgb * visibility, 1.));
}