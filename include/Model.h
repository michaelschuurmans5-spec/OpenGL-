#pragma once

#include <string>
#include <vector>
#include "Mesh.h" // Model gets Vertex / Material / Mesh from here
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model {
public:
    Model() = default;
    explicit Model(const std::string& path) { LoadModel(path); }

    void Draw(const Shader& shader) const {
        for (const auto& mesh : meshes)
            mesh.Draw(shader, GetMaterial(mesh.materialIndex));
    }

    void DrawInstanced(const Shader& shader, const std::vector<glm::mat4>& transforms);

    bool IsLoaded() const { return !meshes.empty(); }

private:
    std::vector<Mesh> meshes;
    std::string directory;

    // OGLDev pattern: one Material per aiScene material slot, sized and
    // filled once in InitMaterials — NOT one per mesh. Meshes reference a
    // slot by index (Mesh::materialIndex), so a material used by 50 meshes
    // is still only ever loaded onto the GPU once, with no dedup search.
    std::vector<Material> materials;

    const Material* GetMaterial(int index) const {
        if (index < 0 || index >= (int)materials.size()) return nullptr;
        return &materials[index];
    }

    void LoadModel(const std::string& path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

    void InitMaterials(const aiScene* scene);
    void LoadMaterialTexture(aiMaterial* mat, aiTextureType type, const std::string& typeName,
        unsigned int& outID, bool& outHas);
};