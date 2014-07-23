#version 150 core

in float vf_radius;

out vec4 out_color;

uniform vec3 axis;
uniform float fadeout;


void main(void)
{
    out_color = vec4(max(fadeout, 1.0 - vf_radius) * axis, 1.0);
}
