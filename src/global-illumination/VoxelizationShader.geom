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

/* outputs to fragment shader */
#ifdef TEXTURED
out vec2 texCoords;
#endif
out vec3 normal;
out flat int dominantAxis;

void main() {
    /* get normal from triangle */
    vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
    vec3 faceNormal = normalize(cross(p1, p2));

    float nDotX = abs(faceNormal.x);
    float nDotY = abs(faceNormal.y);
    float nDotZ = abs(faceNormal.z);

    /* 1 = x axis dominant, 2 = y axis dominant, 3 = z axis dominant */
    dominantAxis = (nDotX >= nDotY && nDotX >= nDotZ) ? 1 : (nDotY >= nDotX && nDotY >= nDotZ) ? 2 : 3;
    mat4 projectionMatrix = dominantAxis == 1 ? projectionMatrixX : dominantAxis == 2 ? projectionMatrixY : projectionMatrixZ;
    
    /* For every vertex sent in vertices */
    for(int i = 0; i < gl_in.length(); i++) {
#ifdef TEXTURED
        texCoords = textureCoords[i];
#endif
        normal = transformedNormal[i];
        gl_Position = projectionMatrix * gl_in[i].gl_Position;
        EmitVertex();
    }
    
    /* Finished */
    EndPrimitive();
}