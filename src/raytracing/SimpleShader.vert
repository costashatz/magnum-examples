layout(location = 0) in highp vec4 position;
#ifdef TEXTURED
layout(location = 1) in mediump vec2 textureCoords;
#endif
layout(location = 2) in mediump vec3 normal;

uniform highp mat4 transformationMatrix;
uniform highp mat4 projectionMatrix;
uniform mediump mat3 normalMatrix;

out highp vec3 transformedPosition;
out mediump vec3 transformedNormal;
#ifdef TEXTURED
out mediump vec2 interpolatedTextureCoords;
#endif

void main(){
    /* transform position */
	highp vec4 transformedPosition4 = transformationMatrix * position;
    transformedPosition = transformedPosition4.xyz / transformedPosition4.w;

    /* transform and normalize normal */
	transformedNormal = normalize(normalMatrix * normal);

    /* update gl_Position */
	gl_Position = projectionMatrix * transformedPosition4;
#ifdef TEXTURED
    /* pass through texture coordinates */
    interpolatedTextureCoords = textureCoords;
#endif
}