#pragma once 

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <string>
#include "Shader.h"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec3 tangent;
};

// OGLDev-style material: one of these exists per aiMaterial in the scene
// (Model::materials.size() == scene->mNumMaterials), NOT one per mesh.
// Meshes just hold an index into this array, so a material shared by many
// meshes is only ever loaded onto the GPU once.
struct Material {
    unsigned int diffuseID = 0;
    unsigned int normalID = 0;
    unsigned int roughnessID = 0;

    bool hasDiffuse = false;
    bool hasNormal = false;
    bool hasRoughness = false;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Index into the owning Model's materials vector. INVALID_MATERIAL (-1)
    // means "no material" (matches OGLDev's INVALID_MATERIAL sentinel).
    static constexpr int INVALID_MATERIAL = -1;
    int materialIndex = INVALID_MATERIAL;

    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds, int matIndex)
        : vertices(std::move(verts)), indices(std::move(inds)), materialIndex(matIndex) {
        SetupMesh();
    }

    ~Mesh() { Cleanup(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept
        : vertices(std::move(other.vertices))
        , indices(std::move(other.indices))
        , materialIndex(other.materialIndex)
        , VAO(other.VAO)
        , VBO(other.VBO)
        , EBO(other.EBO)
        , instanceVBO(other.instanceVBO)
        , instancingSetup(other.instancingSetup) {
        other.VAO = other.VBO = other.EBO = other.instanceVBO = 0;
        other.instancingSetup = false;
    }

    Mesh& operator=(Mesh&& other) noexcept {
        if (this != &other) {
            Cleanup();
            vertices = std::move(other.vertices);
            indices = std::move(other.indices);
            materialIndex = other.materialIndex;
            VAO = other.VAO; VBO = other.VBO; EBO = other.EBO;
            instanceVBO = other.instanceVBO;
            instancingSetup = other.instancingSetup;
            other.VAO = other.VBO = other.EBO = other.instanceVBO = 0;
            other.instancingSetup = false;
        }
        return *this;
    }

    // 'material' is looked up by the caller (Model) via materialIndex and
    // passed in — the Mesh itself never owns or loads textures.
    void Draw(const Shader& shader, const Material* material) const {
        BindMaterial(shader, material);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void DrawInstanced(const Shader& shader, const std::vector<glm::mat4>& transforms, const Material* material) const {
        if (transforms.empty()) return;
        SetupInstancing();

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(glm::mat4), transforms.data(), GL_DYNAMIC_DRAW);

        BindMaterial(shader, material);

        glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0, (GLsizei)transforms.size());

        // NOTE: we deliberately do NOT disable attributes 4-7 here. Enable state
        // lives on this Mesh's own VAO and should stay on permanently once
        // SetupInstancing() has run once - disabling it here (as the old code
        // did) killed the instance matrix on every draw after the first, since
        // SetupInstancing()'s one-time guard never re-enabled it.
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

private:
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    mutable unsigned int instanceVBO = 0;
    mutable bool instancingSetup = false;

    void Cleanup() {
        if (VAO != 0) glDeleteVertexArrays(1, &VAO);
        if (VBO != 0) glDeleteBuffers(1, &VBO);
        if (EBO != 0) glDeleteBuffers(1, &EBO);
        if (instanceVBO != 0) glDeleteBuffers(1, &instanceVBO);
        VAO = VBO = EBO = instanceVBO = 0;
    }

    void BindMaterial(const Shader& shader, const Material* material) const
    {
        bool hasDiffuse = material && material->hasDiffuse;
        bool hasNormal = material && material->hasNormal;
        bool hasRoughness = material && material->hasRoughness;

      

        shader.setBool("hasTexture", hasDiffuse);
        shader.setBool("hasNormalMap", hasNormal);
        shader.setBool("hasRoughnessMap", hasRoughness);

        if (hasDiffuse)
        {
            glActiveTexture(GL_TEXTURE0);
            shader.setInt("texture_diffuse1", 0);
            glBindTexture(GL_TEXTURE_2D, material->diffuseID);
        }

        if (hasNormal)
        {
            glActiveTexture(GL_TEXTURE1);
            shader.setInt("texture_normal1", 1);
            glBindTexture(GL_TEXTURE_2D, material->normalID);
        }

        if (hasRoughness)
        {
            glActiveTexture(GL_TEXTURE2);
            shader.setInt("texture_roughness1", 2);
            glBindTexture(GL_TEXTURE_2D, material->roughnessID);
        }
    }

    void SetupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

        // moved from location 3 -> 7 so it doesn't collide with the instance matrix
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

        glBindVertexArray(0);
    }

    void SetupInstancing() const {
        if (instancingSetup) return;
        glGenBuffers(1, &instanceVBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

        std::size_t vec4Size = sizeof(glm::vec4);
        std::size_t mat4Size = sizeof(glm::mat4);
        for (int i = 0; i < 4; i++) {
            unsigned int attribLocation = 3 + i;   // now matches Prop.vert's `layout(location=3) in mat4`
            glEnableVertexAttribArray(attribLocation);
            glVertexAttribPointer(attribLocation, 4, GL_FLOAT, GL_FALSE, (GLsizei)mat4Size, (void*)(i * vec4Size));
            glVertexAttribDivisor(attribLocation, 1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        instancingSetup = true;
    }
};