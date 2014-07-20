#version 150 core

in float in_pos;

uniform mat4 proj, mv;
uniform float length;
uniform vec3 direction;


void main(void)
{
    vec3 vertex = length * direction * in_pos;

    gl_Position = proj * mv * length * vec4(vertex, 1.0);
}
