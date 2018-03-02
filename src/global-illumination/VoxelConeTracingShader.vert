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

layout(location = 1)
uniform mat4 modelMatrix;

layout(location = 2)
uniform vec3 cameraPosition;

/* outputs to geometry shader */
#ifdef TEXTURED
out vec2 textureCoords;
#endif
out vec3 worldPosition;
out vec3 worldNormal;
out vec3 eyeDir;
out mat3 normalMatrix;

void main() {
    /* transform vertex */
    gl_Position = transformationMatrix * vec4(position, 1.);

    /* compute world position */
    worldPosition = (modelMatrix * vec4(position, 1.)).xyz;

    /* compute world normal */
    /* @todo: check if this is correct */
    normalMatrix = mat3(transpose(inverse(modelMatrix)));
    worldNormal = normalize(normalMatrix * normal);

    /* compute eye direction; un-normalized */
    eyeDir = cameraPosition - worldPosition;

#ifdef TEXTURED
    /* pass through texture coordinates */
    textureCoords = texCoords;
#endif
}