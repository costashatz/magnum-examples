/* uniforms */
layout(location = 0)
uniform float voxelSize;
layout(location = 1)
uniform int voxelDimensions;

/* voxel texture */
layout(binding = 0) uniform sampler3D voxelTexture;

/* outputs to geometry shader */
out vec4 color;

void main() {
    /* calculate the center of voxel */
	vec3 pos;
	pos.x = gl_VertexID % voxelDimensions;
	pos.z = (gl_VertexID / voxelDimensions) % voxelDimensions;
	pos.y = gl_VertexID / (voxelDimensions * voxelDimensions);

    /* get color information */
	color = texture(voxelTexture, pos / voxelDimensions);

    /* set appropriate vertex position */
	gl_Position = vec4(pos - voxelDimensions * 0.5, 1);
}