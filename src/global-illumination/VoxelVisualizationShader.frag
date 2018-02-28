/* inputs from geometry shader */
in vec3 normal;
in vec4 fragColor;

/* outputs */
out vec4 color;

void main() {
	if(fragColor.a < 0.5)
		discard;
    
    /* @todo: add some lighting? */

	color = vec4(fragColor.rgb, 1.);
}