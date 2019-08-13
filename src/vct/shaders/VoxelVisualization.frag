// #version 430

in vec4 voxelColor;

out vec4 fragColor;

void main()
{
	fragColor = vec4(voxelColor);
	// fragColor = vec4(1.f, 0.f, 0.f, 1.f);
}