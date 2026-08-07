#pragma once

#include "Shader.h"
#include "GameObject.h"

// std
#include <vector>
#include <string>

// glm
#include <glm/glm/glm.hpp>

class Triangle {
public:
    Triangle();
    ~Triangle();

    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& lightPos, const glm::vec3& cameraPos) const;
    void SpawnCube(const glm::vec3& spawnPosition);
    void SpawnLight(const glm::vec3& spawnPosition);

    std::vector<GameObject>& GetSceneObjects() { return sceneObjects; }

private:
    // Next Cube spawn 
    std::vector<GameObject> sceneObjects;
    int nextCubeID = 1;
    int nextLightID = 1;

    // Cube mesh
    unsigned int VAO, VBO, EBO;
    // Light source cube mesh (shares VBO/EBO with the cube, own VAO)
    unsigned int lightVAO;
    unsigned int cubeTexture;

    // Ground plane mesh
    unsigned int groundVAO, groundVBO, groundEBO;
    unsigned int groundTexture;

    Shader* myShader;
    Shader* lightCubeShader;
};