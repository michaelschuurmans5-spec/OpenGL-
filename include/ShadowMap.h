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
        glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, prevSplit, nextSplit);
        glm::mat4 invVP = glm::inverse(proj * viewMatrix);

        // 8 corners of sub-frustum in world space
        std::vector<glm::vec4> corners;
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

        glm::vec3 center(0.0f);
        for (const auto& v : corners) center += glm::vec3(v);
        center /= static_cast<float>(corners.size());

        glm::mat4 lightView = glm::lookAt(center - lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max(), maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max(), maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max(), maxZ = std::numeric_limits<float>::lowest();

        for (const auto& v : corners) {
            glm::vec4 tr = lightView * v;
            minX = std::min(minX, tr.x); maxX = std::max(maxX, tr.x);
            minY = std::min(minY, tr.y); maxY = std::max(maxY, tr.y);
            minZ = std::min(minZ, tr.z); maxZ = std::max(maxZ, tr.z);
        }

        // Pull back near plane to capture shadow casters outside view frustum
        constexpr float zMult = 10.0f;
        if (minZ < 0) minZ *= zMult; else minZ /= zMult;
        if (maxZ < 0) maxZ /= zMult; else maxZ *= zMult;

        glm::mat4 lightOrtho = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        shadowMatrices[index] = lightOrtho * lightView;
    }
};
