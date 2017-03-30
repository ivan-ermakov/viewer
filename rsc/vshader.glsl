#version 450

uniform mat4 mvp_matrix;

in vec3 vPos;
in vec3 vNormal;

out vec3 fragVert;
out vec3 fragNormal;

void main()
{
    fragVert = vec3(vPos);
    fragNormal = vec3(vNormal);

    gl_Position = mvp_matrix * vec4(vPos, 1.);
}
