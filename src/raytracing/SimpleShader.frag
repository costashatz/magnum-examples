uniform highp vec3 lightPosition;
uniform lowp vec4 lightColor;

uniform mediump float shininess;

#ifdef AMBIENT_TEXTURE
layout(binding = 0)
uniform lowp sampler2D ambientTexture;
#endif

uniform lowp vec4 ambientColor;

#ifdef DIFFUSE_TEXTURE
layout(binding = 1)
uniform lowp sampler2D diffuseTexture;
#endif

uniform lowp vec4 diffuseColor;

#ifdef SPECULAR_TEXTURE
layout(binding = 2)
uniform lowp sampler2D specularTexture;
#endif

uniform lowp vec4 specularColor;

uniform highp mat4 cameraMatrix;

in highp vec3 transformedPosition;
in mediump vec3 transformedNormal;
#if defined(AMBIENT_TEXTURE) || defined(DIFFUSE_TEXTURE) || defined(SPECULAR_TEXTURE) || defined(TEXTURED)
in mediump vec2 interpolatedTextureCoords;
#endif

// struct Object{
//     float r, g, b;
// };

// // Arrays of vec3s have padding on NV driver and I think that it should have even when using std430.

// // 1. If the member is a scalar consuming N basic machine units, the base align-
// // ment is N .
// // 2. If the member is a two- or four-component vector with components consum-
// // ing N basic machine units, the base alignment is 2N or 4N , respectively.
// // 3. If the member is a three-component vector with components consuming N
// // basic machine units, the base alignment is 4N .
// // 4. If the member is an array of scalars or vectors, the base alignment and array
// // stride are set to match the base alignment of a single array element, according
// // to rules (1), (2), and (3), and rounded up to the base alignment of a vec4. The
// // array may have padding at the end; the base offset of the member following
// // the array is rounded up to the next multiple of the base alignment.

// // Shader storage blocks (see section 7.8) also support the std140 layout qual-
// // ifier, as well as a std430 qualifier not supported for uniform blocks. When using
// // the std430 storage layout, shader storage blocks will be laid out in buffer storage
// // identically to uniform and shader storage blocks using the std140 layout, except
// // that the base alignment and stride of arrays of scalars and vectors in rule 4 and of
// // structures in rule 9 are not rounded up a multiple of the base alignment of a vec4.

// layout(std430, binding = 3) buffer PrimitiveBuffer{
// 	Object objects[];
// };

out lowp vec4 color;

void main() {
    lowp vec4 finalAmbientColor =
        #ifdef AMBIENT_TEXTURE
        texture(ambientTexture, interpolatedTextureCoords)*
        #endif
        ambientColor;
    lowp vec4 finalDiffuseColor =
        #ifdef DIFFUSE_TEXTURE
        texture(diffuseTexture, interpolatedTextureCoords)*
        #endif
        diffuseColor;
    lowp vec4 finalSpecularColor =
        #ifdef SPECULAR_TEXTURE
        texture(specularTexture, interpolatedTextureCoords)*
        #endif
        specularColor;

    /* Ambient color */
    color = finalAmbientColor;

    /* transform light position */
    highp vec3 transformedLightPosition = vec3(cameraMatrix * vec4(lightPosition, 1.));

    highp vec3 lightDirection = normalize(transformedLightPosition - transformedPosition);

    /* Add diffuse color */
    lowp float intensity = max(0.0, dot(transformedNormal, lightDirection));
    color += finalDiffuseColor * lightColor * intensity;

    /* Add specular color, if needed */
    if(intensity > 0.001) {
        highp vec3 reflection = reflect(-lightDirection, transformedNormal);
        mediump float specularity = pow(max(0.0, dot(normalize(-transformedPosition), reflection)), shininess);
        color += finalSpecularColor*specularity;
    }

    // int index = 2;
    // color = vec4(objects[index].r, objects[index].g, objects[index].b, 1);
}
