#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    // If texture color is non-zero, use it; otherwise fallback to gray
    gAlbedoSpec.rgb = (texColor.rgb != vec3(0.0)) ? texColor.rgb : vec3(0.5);
    gAlbedoSpec.a = 0.5; // Specular
}