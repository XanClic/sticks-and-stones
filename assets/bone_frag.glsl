#version 150 core

in vec3 vf_normal;

out vec4 out_col;

uniform vec3 color;


void main(void)
{
    vec3 normal = normalize(vf_normal);

    float diff_co = max(0.1, dot(normal, vec3(0.0, 0.0, 1.0)));
    float spec_co = pow(max(0.0, dot(reflect(vec3(0.0, 0.0, 1.0), normal), vec3(0.0, 0.0, -1.0))), 5.0);

    out_col = vec4(diff_co * color + spec_co * vec3(1.0, 1.0, 1.0), 1.0);
}
