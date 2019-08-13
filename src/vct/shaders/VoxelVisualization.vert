// #version 430
#extension GL_ARB_shader_image_load_store : require

out vec4 albedo;

layout(binding = 0, rgba8) uniform readonly image3D voxel;

uniform uint volumeDimension;

void main()
{
	vec3 position = vec3
	(
		gl_VertexID % volumeDimension,
		(gl_VertexID / volumeDimension) % volumeDimension,
		gl_VertexID / (volumeDimension * volumeDimension)
	);

	ivec3 texPos = ivec3(position);
	albedo = imageLoad(voxel, texPos);
	// albedo.rgb *= 0.9*position.rgb/float(volumeDimension);
	// albedo.rgb = 0.*albedo.rgb + 0.9*position/float(volumeDimension);

	gl_Position = vec4(position, 1.0f);
}