out vec2 textureCoordinates;
 
void main()
{
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    textureCoordinates.x = (x + 1.0) * 0.5;
    textureCoordinates.y = (y + 1.0) * 0.5;
    gl_Position = vec4(x, y, 0, 1);
}