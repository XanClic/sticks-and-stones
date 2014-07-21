#version 150 core

layout(points) in;
layout(triangle_strip, max_vertices=16) out;

out float gf_radius;

uniform mat4 mv, proj;
uniform vec3 axis;
uniform float l1, l2;


void main(void)
{
    float angle = l1, angle_diff = (l2 - l1) / 8.0;

    vec3 sin_axis = vec3(axis.y + axis.z, 0.0, axis.x);
    vec3 cos_axis = vec3(0.0, axis.x + axis.z, axis.y);

    mat4 mvp = proj * mv;

    for (int i = 0; i < 4; i++) {
        gf_radius = 1.0;
        gl_Position = mvp * vec4(sin_axis * sin(angle) + cos_axis * cos(angle), 1.0);
        EmitVertex();

        gf_radius = 0.0;
        gl_Position = mvp * vec4(0.0, 0.0, 0.0, 1.0);
        EmitVertex();

        gf_radius = 1.0;
        angle += angle_diff;
        gl_Position = mvp * vec4(sin_axis * sin(angle) + cos_axis * cos(angle), 1.0);
        EmitVertex();

        gf_radius = 1.0;
        angle += angle_diff;
        gl_Position = mvp * vec4(sin_axis * sin(angle) + cos_axis * cos(angle), 1.0);
        EmitVertex();

        EndPrimitive();
    }
}
