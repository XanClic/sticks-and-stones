#version 150 core

in vec2 in_rad_ang;

out float vf_radius;

uniform mat4 mv, proj;
uniform vec3 axis;
uniform float l1, l2;


void main(void)
{
    float angle = mix(l1, l2, in_rad_ang.y);

    vec3 sin_axis = vec3(axis.y + axis.z, 0.0, axis.x);
    vec3 cos_axis = vec3(0.0, axis.x + axis.z, axis.y);

    vf_radius = in_rad_ang.x;
    gl_Position = proj * mv * vec4(in_rad_ang.x * (sin_axis * sin(angle) + cos_axis * cos(angle)), 1.0);
}
