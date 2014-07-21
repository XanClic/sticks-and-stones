#version 150 core

in float gf_radius;

out vec4 out_color;

uniform vec3 axis;


void main(void)
{
    out_color = vec4(max(0.3, 1.0 - gf_radius) * axis, 1.0);
}
