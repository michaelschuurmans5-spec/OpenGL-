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

    // Generic basic-shape spawn used by the Objects > Shapes properties panel
    // (Create button) and by dragging a shape from that menu into the viewport.
    void SpawnShape(ObjectType type, const glm::vec3& spawnPosition, const std::string& customName,
        const glm::vec3& baseColor, TextureSlot textureSlot);

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

    // Basic-shape meshes used by the Objects > Shapes drag-and-drop panel
    unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
    unsigned int sphereIndexCount = 0;

    unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
    unsigned int cylinderIndexCount = 0;

    unsigned int planeVAO = 0, planeVBO = 0, planeEBO = 0; // unit quad, distinct from the scene's Ground plane

    unsigned int prismVAO = 0, prismVBO = 0, prismEBO = 0;
    unsigned int prismIndexCount = 0;

    Shader* myShader;
    Shader* lightCubeShader;
};