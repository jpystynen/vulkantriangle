#version 430 core

layout(location = 0) out vec4 out_color;

void main(void)
{
    out_color = vec4(
        float(gl_FragCoord.x)/1600.0f,
        float(gl_FragCoord.y)/900.0f, 
        float(gl_FragCoord.x)/1600.0f, 1.0);
}
