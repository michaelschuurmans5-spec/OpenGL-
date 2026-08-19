#version 330 core

in vec2 screenUV;

out vec4 FragColor;

uniform mat4 view;
uniform mat4 projection;

uniform vec3 sunDirection;

uniform float skyBrightness;
uniform float horizonHaze;

uniform float cloudCoverage;
uniform float cloudDensity;
uniform float cloudSpeed;
uniform float cloudHeight;

uniform int cloudsEnabled;

// --------------------------------------------------
// Convert screen position into a world-space ray.
// --------------------------------------------------
vec3 GetWorldRay(vec2 uv)
{
    // Convert UV [0,1] to Clip Space NDC [-1,1]
    vec4 rayClip = vec4(uv * 2.0 - 1.0, -1.0, 1.0);

    // Transform clip space to view space
    vec4 rayView = inverse(projection) * rayClip;
    rayView = vec4(rayView.xy, -1.0, 0.0); // Direction vector pointing forward into view space

    // Transform view space to world space using inverse view matrix
    vec3 rayWorld = (inverse(view) * rayView).xyz;
    
    return normalize(rayWorld);
}

// --------------------------------------------------
// Main
// --------------------------------------------------
void main()
{
    vec3 ray = GetWorldRay(screenUV);
    vec3 normSunDir = normalize(sunDirection);

    // Sun alignment dot product
    float sunHeight = max(dot(ray, normSunDir), 0.0);

    // Gradient palette
    vec3 zenithColor  = vec3(0.08, 0.28, 0.60);
    vec3 horizonColor = vec3(0.65, 0.80, 0.95);

    // Gradient factor based on vertical ray direction
    float horizonFactor = clamp(ray.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 skyColor = mix(horizonColor, zenithColor, horizonFactor);

    // Sun disc and atmospheric glow
    float sunGlow = pow(sunHeight, 32.0);
    float sunDisc = pow(sunHeight, 512.0); // Crisp sun core
    
    skyColor += vec3(1.0, 0.55, 0.20) * sunGlow * 0.5;
    skyColor += vec3(1.0, 0.90, 0.70) * sunDisc * 2.0;

    // Horizon haze blending
    float haze = 1.0 - smoothstep(0.0, 0.45, abs(ray.y));
    skyColor = mix(skyColor, horizonColor, haze * horizonHaze * 0.35);

    // Global brightness scale
    skyColor *= skyBrightness;

    FragColor = vec4(skyColor, 1.0);
}