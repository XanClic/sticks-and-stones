#version 150 core

in vec3 in_pos, in_nrm;

out vec3 vf_normal;

uniform mat4 proj, mv;
uniform mat3 nrm_mat;
uniform float length;


void main(void)
{
    // cut short (so it doesn't break the tip)
    vec3 vertex = vec3(in_pos.x, max(0.0, (length - 0.03) * in_pos.y), in_pos.z);
    gl_Position = proj * mv * vec4(vertex, 1.0);

    vf_normal = normalize(nrm_mat * in_nrm);
}
