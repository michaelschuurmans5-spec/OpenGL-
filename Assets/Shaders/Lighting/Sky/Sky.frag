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
    vec4 rayClip = vec4(
        uv * 2.0 - 1.0,
        1.0,
        1.0
    );

    vec4 rayView = inverse(projection) * rayClip;

    rayView = vec4(
        rayView.xy,
        1.0,
        0.0
    );

    return normalize(
        mat3(inverse(view)) * rayView.xyz
    );
}


// --------------------------------------------------
// Main
// --------------------------------------------------

void main()
{
    vec3 ray = GetWorldRay(screenUV);

    float sunHeight = max(
        dot(ray, sunDirection),
        0.0
    );

    // Sky colour.
    vec3 zenithColor = vec3(
        0.08,
        0.28,
        0.60
    );

    vec3 horizonColor = vec3(
        0.65,
        0.80,
        0.95
    );

    float horizon = clamp(
        ray.y * 0.5 + 0.5,
        0.0,
        1.0
    );

    vec3 skyColor = mix(
        horizonColor,
        zenithColor,
        horizon
    );

    // ------------------------------------------------
    // Sun influence
    // ------------------------------------------------

    float sunGlow = pow(
        sunHeight,
        32.0
    );

    skyColor += vec3(
        1.0,
        0.55,
        0.20
    ) * sunGlow * 0.5;

    // ------------------------------------------------
    // Horizon haze
    // ------------------------------------------------

    float haze = 1.0 -
        smoothstep(
            0.0,
            0.45,
            abs(ray.y)
        );

    skyColor = mix(
        skyColor,
        horizonColor,
        haze * horizonHaze * 0.35
    );

    skyColor *= skyBrightness;

    FragColor = vec4(
        skyColor,
        1.0
    );
}