#version 150 core

out vec4 out_col;

uniform vec3 color;


void main(void)
{
    out_col = vec4(color, 1.0);
}
