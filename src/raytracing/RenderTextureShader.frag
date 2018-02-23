layout(binding = 0)
uniform sampler2D rayImage;

in vec2 textureCoordinates;

out vec4 color;

void main() {
    color.rgb = texture(rayImage, textureCoordinates).rgb;
    color.a = 1.;
}