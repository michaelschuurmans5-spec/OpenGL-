#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

// Uniforms driven by your LightSettings UI
uniform vec3 mainSunDir;
uniform vec3 lightColor; // Includes intensity (sunColor * sunIntensity)
uniform float ambientIntensity;
uniform vec3 viewPos;

// CSM Shadow Mapping Uniforms
uniform mat4 view;
uniform sampler2DArray shadowMapArray;
uniform mat4 shadowMatrices[4];
uniform float cascadeSplits[4];
uniform float shadowBiasMin;
uniform float shadowBiasMax;

uniform sampler2D texture_diffuse1; // Set by Model class

float CalculateCSMShadow(vec3 fragPosWorld, vec3 normal, vec3 lightDir)
{
    vec4 fragPosView = view * vec4(fragPosWorld, 1.0);
    float depthValue = abs(fragPosView.z);

    int selectedLayer = 3;
    for (int i = 0; i < 4; ++i) {
        if (depthValue < cascadeSplits[i]) {
            selectedLayer = i;
            break;
        }
    }

    vec4 fragPosLightSpace = shadowMatrices[selectedLayer] * vec4(fragPosWorld, 1.0);
    if (fragPosLightSpace.w <= 0.0) return 0.0;

    vec3 projCoords = (fragPosLightSpace.xyz / fragPosLightSpace.w) * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float bias = max(shadowBiasMax * (1.0 - dot(normal, lightDir)), shadowBiasMin);
    if (selectedLayer == 3) bias *= 1.5;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMapArray, 0));

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMapArray, vec3(projCoords.xy + vec2(x, y) * texelSize, selectedLayer)).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }

    return clamp(shadow / 9.0, 0.0, 1.0);
}

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 sunLightDir = normalize(mainSunDir);

    // 1. Ambient
    vec3 ambient = ambientIntensity * lightColor;

    // 2. Diffuse
    float diff = max(dot(norm, sunLightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // 3. Shadows
    float shadow = CalculateCSMShadow(FragPos, norm, sunLightDir);

    // Combine
    vec3 baseColor = texture(texture_diffuse1, TexCoord).rgb;
    vec3 finalLighting = ambient + (1.0 - shadow) * diffuse;

    FragColor = vec4(finalLighting * baseColor, 1.0);
}