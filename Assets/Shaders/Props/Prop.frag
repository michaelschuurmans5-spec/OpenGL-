#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;
out vec4 FragColor;

uniform sampler2D diffuseTex;
uniform bool hasTexture;
uniform vec3 sunDirection;
uniform vec3 fallbackColor = vec3(0.5, 0.5, 0.5);

void main() {
	vec3 base = hasTexture ? texture(diffuseTex, UV).rgb : fallbackColor;

	vec3 N = normalize(Normal);
	vec3 L = normalize(-sunDirection);
	float diff = max(dot(N, L), 0.0);
	vec3 ambient = base * 0.25;
	vec3 diffuse = base * diff * 0.85;

	FragColor = vec4(ambient + diffuse, 1.0);
}