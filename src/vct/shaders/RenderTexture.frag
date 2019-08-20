layout(binding = 0)
uniform sampler2D textureImage;

layout(location = 0)
uniform int isDepth = 0;

in vec2 textureCoordinates;

out vec4 color;

void main() {
    if(isDepth == 0)
        color.rgb = texture(textureImage, textureCoordinates).rgb;
    else {
        float v = texture(textureImage, textureCoordinates).r;
        color.rgb = vec3(v, v, v);
    }
    color.a = 1.;
}