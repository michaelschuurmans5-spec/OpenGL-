#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in vec3 aTangent;

layout (location = 4) in mat4 instanceModel;

out vec3 FragPos;
out vec2 UV;
out mat3 TBN;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPosition = instanceModel * vec4(aPos, 1.0);

    FragPos = worldPosition.xyz;
    UV = aUV;

    mat3 normalMatrix =
        mat3(instanceModel);

    vec3 N = normalize(normalMatrix * aNormal);

    vec3 T = normalize(normalMatrix * aTangent);

    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    gl_Position =
        projection *
        view *
        worldPosition;
}