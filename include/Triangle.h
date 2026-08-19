#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

#include "Shader.h"
#include "Model.h"
#include "ShadowMap.h"
#include "TerrainParams.h"
#include "Sky.h"

enum class ObjectType {
    Cube,
    Sphere,
    Cylinder,
    Plane,
    Prism,
    Ground,
    Light,
    Terrain,
    Prop
};

inline std::string ToString(ObjectType type) {
    switch (type) {
    case ObjectType::Cube: return "Cube";
    case ObjectType::Sphere: return "Sphere";
    case ObjectType::Cylinder: return "Cylinder";
    case ObjectType::Plane: return "Plane";
    case ObjectType::Prism: return "Prism";
    case ObjectType::Ground: return "Ground";
    case ObjectType::Light: return "Light";
    case ObjectType::Terrain: return "Terrain";
    case ObjectType::Prop: return "Prop";
    }
    return "Unknown";
}

enum class TextureSlot {
    None,
    Container,
    Grass
};

struct GameObject {
    int id = 0;
    ObjectType type = ObjectType::Cube;
    std::string name;
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
    glm::mat4 transformMatrix{ 1.0f };
    glm::vec3 baseColor{ 1.0f };
    TextureSlot textureSlot = TextureSlot::None;
    bool visible = true;
    bool locked = false;
    bool rotates = false;
    bool isBasicShape = true;
    std::string modelName = "";
    unsigned int textureOverride = 0;
};

struct LightSettings {
    glm::vec3 sunColor{ 1.0f, 0.95f, 0.85f };
    float sunIntensity = 1.2f;
    float ambientIntensity = 0.2f;
    float sunAzimuth = 45.0f;
    float sunElevation = 35.0f;
    float shadowBiasMin = 0.0005f;
    float shadowBiasMax = 0.005f;
    bool debugCascades = false;
	float timeOfDay = 12.0f; // 24 hour clock
};

class Sky;

class Triangle {
public:
    Triangle();
    ~Triangle();

    void Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& lightPos, const glm::vec3& cameraPos) const;
   
    void RenderCSMShadowPass(const glm::vec3& cameraPos, const glm::vec3& cameraFront, float aspect, float fovDeg, float nearPlane, float farPlane);
    void DrawGBuffer(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void DrawLightHelpers(const glm::mat4& view, const glm::mat4& projection);

    void SpawnCube(const glm::vec3& spawnPosition);
    void SpawnLight(const glm::vec3& spawnPosition);
    void SpawnShape(ObjectType type, const glm::vec3& spawnPosition, const std::string& customName = "", const glm::vec3& baseColor = glm::vec3(1.0f), TextureSlot textureSlot = TextureSlot::None);
    void SpawnProp(const std::string& modelName, const glm::vec3& spawnPosition);

    void PreviewTerrain(const TerrainParams& params);
    void CommitTerrain();
    void DeleteTerrain();
    float GetTerrainHeightAt(float worldX, float worldZ) const;
    bool HasTerrainPreview() const { return terrainIndexCount > 0; }
    void OnObjectDeleted(int objectId);
    bool IsTerrainCommitted() const { return terrainIndexCount > 0 && terrainObjectId != -1; }


    std::vector<GameObject>& GetSceneObjects() { return sceneObjects; }
    const std::vector<GameObject>& GetSceneObjects() const { return sceneObjects; }

    const std::unordered_map<std::string, std::unique_ptr<Model>>& GetPropModels() const {
        return propModels;
    }

    glm::vec3 GetSunDirection() const {
        float azRad = glm::radians(lightSettings.sunAzimuth);
        float elRad = glm::radians(lightSettings.sunElevation);
        return glm::normalize(glm::vec3(
            cos(elRad) * sin(azRad),
            sin(elRad),
            cos(elRad) * cos(azRad)
        ));
    }

    Sky* sky = nullptr;
    ShadowMap* shadowMap = nullptr;
    LightSettings lightSettings;
    Shader* gShapeShader = nullptr;

private:
    void LoadPropModels();
    void DrawObjectMesh(const GameObject& obj);
    unsigned int LoadTexture(const std::string& path);
    void LoadTextureLibrary();
    void DrawPropsInstanced(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& sunDir, const glm::vec3& cameraPos) const;

    unsigned int VAO = 0, VBO = 0, EBO = 0;
    unsigned int lightVAO = 0;
    unsigned int groundVAO = 0, groundVBO = 0, groundEBO = 0;
    unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0, sphereIndexCount = 0;
    unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0, cylinderIndexCount = 0;
    unsigned int planeVAO = 0, planeVBO = 0, planeEBO = 0;
    unsigned int prismVAO = 0, prismVBO = 0, prismEBO = 0, prismIndexCount = 0;

    unsigned int terrainVAO = 0, terrainVBO = 0, terrainEBO = 0, terrainIndexCount = 0;
    std::vector<float> terrainHeights;
    int terrainHeightRes = 0;
    float terrainHeightSize = 0.0f;
    int terrainObjectId = -1;
    TerrainParams terrainParams;

    unsigned int groundTexture = 0;
    unsigned int cubeTexture = 0;

    Shader* myShader = nullptr;
    Shader* lightCubeShader = nullptr;
    Shader* csmShader = nullptr;
    Shader* propShader = nullptr;

    std::vector<GameObject> sceneObjects;
    std::unordered_map<std::string, std::unique_ptr<Model>> propModels;
    std::unordered_map<std::string, unsigned int> textureLibrary;

    int nextCubeID = 1;
    int nextLightID = 1;
};