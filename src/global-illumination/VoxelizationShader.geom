layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

/* inputs from vertex shader; stored in arrays */
#ifdef TEXTURED
in vec2 textureCoords[];
#endif
in vec3 transformedNormal[];

/* uniforms */
layout(location = 1)
uniform mat4 projectionMatrixX;

layout(location = 2)
uniform mat4 projectionMatrixY;

layout(location = 3)
uniform mat4 projectionMatrixZ;

layout(location = 5)
uniform int voxelDimensions;

layout(location = 6)
uniform float voxelSize;

/* outputs to fragment shader */
#ifdef TEXTURED
out vec2 texCoords;
#endif
out vec3 normal;
out flat int dominantAxis;
out flat int voxelDims;
out vec4 boundingBox;
out vec3 vertexPos;

void main() {
    /* pass through voxel dimensions */
    voxelDims = voxelDimensions;
    /* get normal from triangle */
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 faceNormal = normalize(cross(p1, p2));

    float nDotX = abs(faceNormal.x);
    float nDotY = abs(faceNormal.y);
    float nDotZ = abs(faceNormal.z);

    /* 1 = x axis dominant, 2 = y axis dominant, 3 = z axis dominant */
    dominantAxis = (nDotX > nDotY && nDotX > nDotZ) ? 1 : (nDotY > nDotX && nDotY > nDotZ) ? 2 : 3;
    mat4 projectionMatrix = dominantAxis == 1 ? projectionMatrixX : dominantAxis == 2 ? projectionMatrixY : projectionMatrixZ;

    // /* calculate vertex positions */
    // vec4 positions[3];
    // positions[0] = projectionMatrix * gl_in[0].gl_Position;
    // positions[1] = projectionMatrix * gl_in[1].gl_Position;
    // positions[2] = projectionMatrix * gl_in[2].gl_Position;

    // /* enlarge the triangle to enable conservative rasterization */
    // vec4 AABB;
	// vec2 hPixel = vec2(1. / voxelDimensions, 1. / voxelDimensions);
	// float pl = 1.4142135637309; //1.4142135637309 / voxelDimensions; //voxelSize;

	// /* calculate AABB of this triangle */
	// AABB.xy = positions[0].xy;
	// AABB.zw = positions[0].xy;

	// AABB.xy = min(positions[1].xy, AABB.xy);
	// AABB.zw = max(positions[1].xy, AABB.zw);

	// AABB.xy = min(positions[2].xy, AABB.xy);
	// AABB.zw = max(positions[2].xy, AABB.zw);

	// /* Enlarge half-pixel */
	// AABB.xy -= hPixel;
	// AABB.zw += hPixel;

    // /* pass it to the fragment shader */
    // boundingBox = AABB;

	// /* find 3 triangle edge plane */
    // vec3 e0 = vec3(positions[1].xy - positions[0].xy, 0);
	// vec3 e1 = vec3(positions[2].xy - positions[1].xy, 0);
	// vec3 e2 = vec3(positions[0].xy - positions[2].xy, 0);
	// vec3 n0 = cross(e0, vec3(0, 0, 1));
	// vec3 n1 = cross(e1, vec3(0, 0, 1));
	// vec3 n2 = cross(e2, vec3(0, 0, 1));

	// /* dilate the triangle */
	// positions[0].xy = positions[0].xy + pl * ((e2.xy / dot(e2.xy, n0.xy)) + (e0.xy / dot(e0.xy, n2.xy)));
	// positions[1].xy = positions[1].xy + pl * ((e0.xy / dot(e0.xy, n1.xy)) + (e1.xy / dot(e1.xy, n0.xy)));
	// positions[2].xy = positions[2].xy + pl * ((e1.xy / dot(e1.xy, n2.xy)) + (e2.xy / dot(e2.xy, n1.xy)));

    /* For every vertex sent in vertices */
    for(int i = 0; i < gl_in.length(); i++) {
#ifdef TEXTURED
        texCoords = textureCoords[i];
#endif
        normal = transformedNormal[i];
        gl_Position = projectionMatrix * gl_in[i].gl_Position; //positions[i]; //projectionMatrix * gl_in[i].gl_Position;
        // vertexPos = positions[i].xyz;
        EmitVertex();
    }

    /* Finished */
    EndPrimitive();
}