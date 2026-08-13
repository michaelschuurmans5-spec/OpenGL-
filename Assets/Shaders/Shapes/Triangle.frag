#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

#define MAX_LIGHTS 8

// Lighting Uniforms
uniform vec3 lightPositions[MAX_LIGHTS];
uniform int numLights;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float ambientIntensity;

uniform sampler2D texture1;
uniform bool useTexture;

// CSM Shadow Mapping Uniforms
uniform mat4 view;                             // Active camera view matrix
uniform sampler2DArray shadowMapArray;         // Bound to Texture Unit 5
uniform mat4 shadowMatrices[4];                // Light matrices for the 4 cascades
uniform float cascadeSplits[4];                // Depth splits in camera view-space
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

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // Dynamic slope bias controlled by ImGui
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

    vec3 ambient = ambientIntensity * lightColor;
    vec3 diffuse = vec3(0.0);
    vec3 specular = vec3(0.0);

    int layerIndex = 0;

    for (int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lightPositions[i] - FragPos);

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffComp = diff * lightColor;

        float specularStrength = 0.3;
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        vec3 specComp = specularStrength * spec * lightColor;

        if (i == 0) {
            float shadow = CalculateCSMShadow(FragPos, norm, lightDir, layerIndex);
            diffuse += (1.0 - shadow) * diffComp;
            specular += (1.0 - shadow) * specComp;
        } else {
            diffuse += diffComp;
            specular += specComp;
        }
    }

    vec3 baseColor = useTexture ? texture(texture1, TexCoord).rgb : objectColor;
    vec3 finalLighting = clamp(ambient + diffuse + specular, 0.0, 1.0);
    vec3 colorOut = finalLighting * baseColor;

    // Optional Debug Tint for Cascades
    if (debugCascades) {
        vec3 debugColors[4] = vec3[4](
            vec3(1.0, 0.2, 0.2), // Red = Near
            vec3(0.2, 1.0, 0.2), // Green = Medium
            vec3(0.2, 0.4, 1.0), // Blue = Far
            vec3(1.0, 1.0, 0.2)  // Yellow = Farthest
        );
        colorOut = mix(colorOut, debugColors[layerIndex], 0.35);
    }

    FragColor = vec4(colorOut, 1.0);
}