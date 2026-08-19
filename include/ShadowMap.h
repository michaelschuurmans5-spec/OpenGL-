 #pragma once


#include <glad/glad.h>

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class ShadowMap {
public:
    unsigned int fbo = 0;
    unsigned int depthArrayTexture = 0;


    unsigned int shadowResolution = 2048;
    std::vector<float> cascadeSplits;
    std::vector<glm::mat4> shadowMatrices;

    // Create 2D Array Texture for 4 shadow cascades
    void Init(unsigned int cascadeCount = 4) {
        glGenFramebuffers(1, &fbo);

        glGenTextures(1, &depthArrayTexture);
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthArrayTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F,
            shadowResolution, shadowResolution, cascadeCount,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        constexpr float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthArrayTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        cascadeSplits.resize(cascadeCount);
        shadowMatrices.resize(cascadeCount);
    }

    // Calculates Practical Split Scheme for cascade frustums
    void UpdateCascades(float nearPlane, float farPlane, float fovDeg, float aspect,
        const glm::vec3& lightDir, const glm::mat4& viewMatrix,
        float lambda = 0.5f)
    {
        size_t cascadeCount = cascadeSplits.size();
        std::vector<float> splitDistances(cascadeCount + 1);
        splitDistances[0] = nearPlane;
        splitDistances[cascadeCount] = farPlane;

        // Practical Split Scheme Formula: logarithmic + linear interpolation
        for (size_t i = 1; i < cascadeCount; ++i) {
            float p = static_cast<float>(i) / static_cast<float>(cascadeCount);
            float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
            float linSplit = nearPlane + (farPlane - nearPlane) * p;
            splitDistances[i] = lambda * logSplit + (1.0f - lambda) * linSplit;
            cascadeSplits[i - 1] = splitDistances[i];
        }
        cascadeSplits[cascadeCount - 1] = farPlane;

        // Calculate tight light projection matrix for each cascade
        glm::mat4 invViewProj = glm::inverse(
            glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane) * viewMatrix
        );

        for (size_t i = 0; i < cascadeCount; ++i) {
            CalculateCascadeMatrix(i, splitDistances[i], splitDistances[i + 1],
                fovDeg, aspect, lightDir, viewMatrix);
        }
    }

private:
    void CalculateCascadeMatrix(size_t index, float prevSplit, float nextSplit,
        float fovDeg, float aspect,
        const glm::vec3& lightDir, const glm::mat4& viewMatrix)
    {
        // 1. Calculate perspective projection for current cascade split
        glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, prevSplit, nextSplit);

        // 2. Obtain world-space frustum corners
        glm::mat4 invVP = glm::inverse(proj * viewMatrix);
        std::vector<glm::vec4> corners;
        corners.reserve(8);

        for (unsigned int x = 0; x < 2; ++x) {
            for (unsigned int y = 0; y < 2; ++y) {
                for (unsigned int z = 0; z < 2; ++z) {
                    glm::vec4 pt = invVP * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f
                    );
                    corners.push_back(pt / pt.w);
                }
            }
        }

        // 3. Compute geometric center
        glm::vec3 center(0.0f);
        for (const auto& v : corners) {
            center += glm::vec3(v);
        }
        center /= static_cast<float>(corners.size());

        // 4. Calculate bounding sphere radius (rotation-invariant)
        float radius = 0.0f;
        for (const auto& v : corners) {
            float distance = glm::length(glm::vec3(v) - center);
            radius = std::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        // 5. Build directional light view matrix
        glm::vec3 normalizedLightDir = glm::normalize(lightDir);
        glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(normalizedLightDir, upVector)) > 0.99f) {
            upVector = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        // Set origin far enough back to catch shadow casters outside the immediate view frustum
        glm::mat4 lightView = glm::lookAt(center - normalizedLightDir * radius, center, upVector);

        // 6. Extend Near/Far Z-bounds to prevent near-plane shadow clipping
        float zBufferPadding = 200.0f;
        glm::mat4 lightOrtho = glm::ortho(-radius, radius, -radius, radius, -zBufferPadding, radius + zBufferPadding);

        // 7. Apply Texel Snapping to eliminate shimmering edges
        glm::mat4 shadowMatrix = lightOrtho * lightView;
        glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin *= (static_cast<float>(shadowResolution) / 2.0f);

        glm::vec4 roundedOrigin = glm::round(shadowOrigin);
        glm::vec4 roundOffset = (roundedOrigin - shadowOrigin) * (2.0f / static_cast<float>(shadowResolution));
        roundOffset.z = 0.0f;
        roundOffset.w = 0.0f;

        lightOrtho = glm::translate(glm::mat4(1.0f), glm::vec3(roundOffset)) * lightOrtho;

        shadowMatrices[index] = lightOrtho * lightView;
    }
};