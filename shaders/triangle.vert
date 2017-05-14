#version 430 core

void main(void)
{
    const vec2 vertices[] =
    {
        vec2(-0.5, 0.0),
        vec2( 0.0, 1.0),
        vec2( 0.5, 0.0)
    };
    
    const vec2 vert = vertices[gl_VertexIndex % 3];
    gl_Position = vec4(vert, 0.5, 1.0);
}
