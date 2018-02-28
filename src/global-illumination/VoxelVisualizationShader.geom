layout(points) in;
layout(triangle_strip, max_vertices = 36) out;

/* inputs from vertex shader */
in vec4 color[];

/* uniforms */
layout(location = 2)
uniform mat4 transformationMatrix;

/* outputs to fragment shader */
out vec4 fragColor;
out vec3 normal;

void main() {
	fragColor = color[0];
    /* optimization to run faster */
    if(fragColor.a < 0.5)
        return;

	vec4 v1 = transformationMatrix * (gl_in[0].gl_Position + vec4(-0.5, 0.5, 0.5, 0));
	vec4 v2 = transformationMatrix * (gl_in[0].gl_Position + vec4(0.5, 0.5, 0.5, 0));
	vec4 v3 = transformationMatrix * (gl_in[0].gl_Position + vec4(-0.5, -0.5, 0.5, 0));
	vec4 v4 = transformationMatrix * (gl_in[0].gl_Position + vec4(0.5, -0.5, 0.5, 0));
	vec4 v5 = transformationMatrix * (gl_in[0].gl_Position + vec4(-0.5, 0.5, -0.5, 0));
	vec4 v6 = transformationMatrix * (gl_in[0].gl_Position + vec4(0.5, 0.5, -0.5, 0));
	vec4 v7 = transformationMatrix * (gl_in[0].gl_Position + vec4(-0.5, -0.5, -0.5, 0));
	vec4 v8 = transformationMatrix * (gl_in[0].gl_Position + vec4(0.5, -0.5, -0.5, 0));

	/*
	        v5 _____________ v6
	          /|           /|
	         / |          / |
	        /  |         /  |
	       /   |        /   |
	   v1 /____|_______/ v2 |
	      |    |       |    |
	      |    |_v7____|____| v8
	      |   /        |   /
	      |  /         |  /  
	      | /          | /  
	   v3 |/___________|/ v4
	*/

	// +Z
    normal = vec3(0, 0, 1);
    gl_Position = v1;
    EmitVertex();
    gl_Position = v3;
    EmitVertex();
	gl_Position = v4;
    EmitVertex();
    EndPrimitive();
    gl_Position = v1;
    EmitVertex();
    gl_Position = v4;
    EmitVertex();
	gl_Position = v2;
    EmitVertex();
    EndPrimitive();

    // -Z
    normal = vec3(0, 0, -1);
    gl_Position = v6;
    EmitVertex();
    gl_Position = v8;
    EmitVertex();
	gl_Position = v7;
    EmitVertex();
    EndPrimitive();
    gl_Position = v6;
    EmitVertex();
    gl_Position = v7;
    EmitVertex();
	gl_Position = v5;
    EmitVertex();
    EndPrimitive();

    // +X
    normal = vec3(1, 0, 0);
    gl_Position = v2;
    EmitVertex();
    gl_Position = v4;
    EmitVertex();
	gl_Position = v8;
    EmitVertex();
    EndPrimitive();
    gl_Position = v2;
    EmitVertex();
    gl_Position = v8;
    EmitVertex();
	gl_Position = v6;
    EmitVertex();
    EndPrimitive();

    // -X
    normal = vec3(-1, 0, 0);
    gl_Position = v5;
    EmitVertex();
    gl_Position = v7;
    EmitVertex();
	gl_Position = v3;
    EmitVertex();
    EndPrimitive();
    gl_Position = v5;
    EmitVertex();
    gl_Position = v3;
    EmitVertex();
	gl_Position = v1;
    EmitVertex();
    EndPrimitive();

    // +Y
    normal = vec3(0, 1, 0);
    gl_Position = v5;
    EmitVertex();
    gl_Position = v1;
    EmitVertex();
	gl_Position = v2;
    EmitVertex();
    EndPrimitive();
    gl_Position = v5;
    EmitVertex();
    gl_Position = v2;
    EmitVertex();
	gl_Position = v6;
    EmitVertex();
    EndPrimitive();

    // -Y
    normal = vec3(0, -1, 0);
    gl_Position = v3;
    EmitVertex();
    gl_Position = v7;
    EmitVertex();
	gl_Position = v8;
    EmitVertex();
    EndPrimitive();
    gl_Position = v3;
    EmitVertex();
    gl_Position = v8;
    EmitVertex();
	gl_Position = v4;
    EmitVertex();
    EndPrimitive();
}