layout(binding = 0)
uniform sampler2D textureImage;

in vec2 textureCoordinates;

out vec4 color;

void main() {
    color.rgb = texture(textureImage, textureCoordinates).rgb;
    color.a = 1.;
}