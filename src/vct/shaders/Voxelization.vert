// #version 430

layout(location = 0)
uniform mat4 transformationMatrix;
layout(location = 1)
uniform mat3 normalMatrix;

// TO-DO: Add textures

layout(location = 0) in vec4 position;
layout(location = 2) in vec3 normal;

out vec3 transformedNormal;

void main() {
    /* Transformed vertex position */
    vec4 transformedPosition = transformationMatrix * position;

    /* Transformed normal vector */
    transformedNormal = normalMatrix * normal;

    /* return the position */
    gl_Position = transformedPosition;
}