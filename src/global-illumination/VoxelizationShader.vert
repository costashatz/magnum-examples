/* inputs */
layout(location = 0)
in vec3 position;

#ifdef TEXTURED
layout(location = 1)
in vec2 texCoords;
#endif

layout(location = 2)
in vec3 normal;

/* uniforms */
layout(location = 0)
uniform mat4 transformationMatrix;

/* outputs to geometry shader */
#ifdef TEXTURED
out vec2 textureCoords;
#endif
out vec3 transformedNormal;

void main() {
    /* transform vertex */
    gl_Position = transformationMatrix * vec4(position, 1.);

    /* transform normal */
    transformedNormal = normalize(mat3(transpose(inverse(transformationMatrix))) * normal);

#ifdef TEXTURED
    /* pass through texture coordinates */
    textureCoords = texCoords;
#endif
}