#version 330 core
out vec4 FragColor;

in vec2 TexCoords; // Passed from Screen Quad Vertex Shader

// G-Buffer Samplers
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

// Light Uniforms
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform float ambientIntensity;
uniform float sunIntensity;
uniform vec3 viewPos;

// CSM Uniforms
uniform mat4 view;
uniform sampler2DArray shadowMapArray;
uniform mat4 shadowMatrices[4];
uniform float cascadeSplits[4];
uniform float shadowBiasMin;
uniform float shadowBiasMax;
uniform bool debugCascades;

float CalculateCSMShadow(vec3 fragPosWorld, vec3 normal, vec3 lightDir, out int selectedLayer)
{
    vec4 fragPosView = view * vec4(fragPosWorld, 1.0);
    float depthValue = abs(fragPosView.z);

    selectedLayer = 3;
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
    // 1. Fetch data from G-Buffer textures
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal  = texture(gNormal, TexCoords).rgb;
    vec3 Albedo  = texture(gAlbedoSpec, TexCoords).rgb;

    // Skip lighting on empty sky pixels (where position was unwritten/zeroed)
    if (length(FragPos) < 0.001) {
        discard;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 sunLightDir = normalize(dirLightDir);

    // 2. Ambient & Direct Lighting
    vec3 ambient = ambientIntensity * dirLightColor;
    float sunDiff = max(dot(norm, sunLightDir), 0.0);
    vec3 diffuse = sunDiff * dirLightColor * sunIntensity;

    // 3. CSM Shadow
    int layerIndex = 0;
    float shadow = CalculateCSMShadow(FragPos, norm, sunLightDir, layerIndex);

    vec3 finalLighting = ambient + (1.0 - shadow) * diffuse;
    vec3 colorOut = finalLighting * Albedo;

    // Debug Cascade Layers
    if (debugCascades) {
        vec3 debugColors[4] = vec3[4](
            vec3(1.0, 0.2, 0.2),
            vec3(0.2, 1.0, 0.2),
            vec3(0.2, 0.4, 1.0),
            vec3(1.0, 1.0, 0.2)
        );
        colorOut = mix(colorOut, debugColors[layerIndex], 0.35);
    }

    FragColor = vec4(colorOut, 1.0);
}