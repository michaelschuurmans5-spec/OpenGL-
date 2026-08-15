#pragma once

#include "Shader.h"
#include "GameObject.h"
#include "TerrainParams.h"
#include "ShadowMap.h"
#include "Sky.h"
#include "Model.h"

// std
#include <vector>
#include <string>


#include <glm/glm/glm.hpp>
#include <unordered_map>


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

    // ---------------------------------------------------------------
    // Level Designer > Terrain Generator
    // ---------------------------------------------------------------
    void PreviewTerrain(const TerrainParams& params);
    float GetTerrainHeightAt(float worldX, float worldZ) const;
    void CommitTerrain();
    void DeleteTerrain();
    void OnObjectDeleted(int objectId);
   

    bool HasTerrainPreview() const { return terrainIndexCount > 0; }
    bool IsTerrainCommitted() const { return terrainObjectId != -1; }
    const TerrainParams& GetTerrainParams() const { return terrainParams; }

    // ShadowMap
    ShadowMap* shadowMap = nullptr;
    Shader* csmShader = nullptr;

    // Sky/Atmosphere 
    Sky* sky = nullptr;

    // Wiring into Triangle 
    std::unordered_map<std::string, Model> propModels; // key = prop name, e.g. "Rock01"
    Shader* propShader = nullptr;

    void LoadPropModels();
    void SpawnProp(const std::string& modelName, const glm::vec3& spawnPosition);


    // Lighting & Atmosphere Controls
    
    struct LightSettings {
        float sunAzimuth = 45.0f;     // Horizontal angle around scene (0° - 360°)
        float sunElevation = 45.0f;   // Vertical height in sky (5° - 90°)
        glm::vec3 sunColor = glm::vec3(1.0f, 0.98f, 0.9f); // Slightly warm daylight
        float sunIntensity = 1.0f;
        float ambientIntensity = 0.2f;

        // Shadow parameters
        float shadowBiasMin = 0.0005f;
        float shadowBiasMax = 0.005f;
        bool debugCascades = false;
    } lightSettings;

    // Helper to compute world space light direction vector from spherical angles
    glm::vec3 GetSunDirection() const {
        float azRad = glm::radians(lightSettings.sunAzimuth);
        float elRad = glm::radians(lightSettings.sunElevation);

        glm::vec3 dir;
        dir.x = cos(elRad) * sin(azRad);
        dir.y = sin(elRad);
        dir.z = cos(elRad) * cos(azRad);
        return glm::normalize(dir);
    }

private:
    // Scene Object tracking
    std::vector<GameObject> sceneObjects;
    int nextCubeID = 1;
    int nextLightID = 1;

    // Cube mesh
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    // Light source cube mesh (shares VBO/EBO with the cube, own VAO)
    unsigned int lightVAO = 0;
    unsigned int cubeTexture = 0;

    // Ground plane mesh
    unsigned int groundVAO = 0, groundVBO = 0, groundEBO = 0;
    unsigned int groundTexture = 0;

    // Basic-shape meshes used by the Objects > Shapes drag-and-drop panel
    unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
    unsigned int sphereIndexCount = 0;

    unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
    unsigned int cylinderIndexCount = 0;

    unsigned int planeVAO = 0, planeVBO = 0, planeEBO = 0; // unit quad, distinct from the scene's Ground plane

    unsigned int prismVAO = 0, prismVBO = 0, prismEBO = 0;
    unsigned int prismIndexCount = 0;

    // Level Designer > Terrain Generator mesh + state
    unsigned int terrainVAO = 0, terrainVBO = 0, terrainEBO = 0;
    unsigned int terrainIndexCount = 0;
    TerrainParams terrainParams;
    int terrainObjectId = -1; // -1 = generated but not yet committed to the scene

    std::vector<float> terrainHeights;   // CPU copy of the last-generated heightmap (res+1)x(res+1)
    int   terrainHeightRes = 0;         // resolution the heightmap above was built at
    float terrainHeightSize = 0.0f;      // world-space size (matches TerrainParams::size)

    // Shaders
    Shader* myShader = nullptr;
    Shader* lightCubeShader = nullptr;
};