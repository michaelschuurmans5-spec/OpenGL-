#pragma once

#include "Shader.h"
#include "GameObject.h"
#include "TerrainParams.h"

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

    // ---------------------------------------------------------------
    // Level Designer > Terrain Generator
    // ---------------------------------------------------------------
    // Regenerates the terrain mesh from params immediately. Used both for
    // the live slider preview (called every time a slider changes) and as
    // the final rebuild right before/after a Generate confirm. If a
    // terrain is already committed to the scene, it picks up the new mesh
    // automatically since it shares the same GL buffers.
    void PreviewTerrain(const TerrainParams& params);

    // Locks the current preview mesh in as a real scene object (so it
    // shows in the Viewport Manager and can be selected/renamed/deleted
    // like anything else). Safe to call again later - if a terrain is
    // already committed this just re-affirms it rather than duplicating it.
    void CommitTerrain();

    // Removes the terrain from the scene and frees its mesh entirely.
    void DeleteTerrain();

    // Lets the generic Viewport Manager "Delete Object" flow tell Triangle
    // when the committed terrain's GameObject was deleted directly from
    // the outliner, so terrain tracking doesn't go stale. The mesh itself
    // is left alone (it drops back to being an uncommitted live preview).
    void OnObjectDeleted(int objectId);

    bool HasTerrainPreview() const { return terrainIndexCount > 0; }
    bool IsTerrainCommitted() const { return terrainObjectId != -1; }
    const TerrainParams& GetTerrainParams() const { return terrainParams; }

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

    // Level Designer > Terrain Generator mesh + state
    unsigned int terrainVAO = 0, terrainVBO = 0, terrainEBO = 0;
    unsigned int terrainIndexCount = 0;
    TerrainParams terrainParams;
    int terrainObjectId = -1; // -1 = generated but not yet committed to the scene

    Shader* myShader;
    Shader* lightCubeShader;
};