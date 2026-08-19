#pragma once

#include "Shader.h"
#include <glm/glm/glm.hpp>

class Sky {
public:
    Sky();
    ~Sky();

    void Draw(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& sunDirection
    ) const;

    struct Settings {
        float skyBrightness = 1.0f;
        float horizonHaze = 0.65f;

        float cloudCoverage = 0.55f;
        float cloudDensity = 0.70f;
        float cloudSpeed = 0.025f;
        float cloudHeight = 0.60f;

        bool cloudsEnabled = true;
    };

    Settings settings;

private:
    Shader* skyShader = nullptr;

    unsigned int VAO = 0;
    unsigned int VBO = 0;
};