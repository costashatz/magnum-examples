/* inputs from geometry shader */
in vec3 normal;
in vec4 fragColor;

/* outputs */
out vec4 color;

void main() {
	/* ignore not visible objects */
	if(fragColor.a < 0.5)
		discard;

	/* dummy lighting to see the voxels better */
	vec3 normalizedTransformedNormal = normalize(normal);
	/* light 1 */
	vec3 lightDirection = normalize(vec3(0.6, 0.3, 0.5));
	float intensity = dot(normalizedTransformedNormal, lightDirection);
	/* Diffuse color */
	vec3 diffuseColor = fragColor.rgb * max(0., intensity);
	/* light 2 */
	lightDirection = normalize(vec3(-0.6, -0.3, -0.5));
	intensity = dot(normalizedTransformedNormal, lightDirection);
	/* Diffuse color */
	diffuseColor += fragColor.rgb * max(0., intensity);

	color = vec4(diffuseColor, 1.);
}