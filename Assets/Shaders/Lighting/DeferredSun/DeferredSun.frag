#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

uniform vec3 dirLightDir;
uniform vec3 dirLightColor;

void main() {
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    vec3 normal  = texture(gNormal, TexCoords).rgb;
    vec3 albedo  = texture(gAlbedoSpec, TexCoords).rgb;

    // Background pixels (where no geometry wrote a normal) show sky
    if (length(normal) < 0.1) {
        discard;
    }

    // Directional Sun Lighting with Base Ambient Floor
    vec3 lightDir = normalize(-dirLightDir);
    if (length(dirLightDir) < 0.1) lightDir = normalize(vec3(0.5, 1.0, 0.3)); // Default sun dir fallback

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 ambient = vec3(0.3) * albedo; // 30% ambient light guarantee
    vec3 diffuse = diff * albedo * (dirLightColor == vec3(0.0) ? vec3(1.0) : dirLightColor);

    FragColor = vec4(ambient + diffuse, 1.0);
}