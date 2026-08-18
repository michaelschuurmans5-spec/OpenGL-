#include "Model.h"
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <stb_image.h>

void Model::LoadModel(const std::string& path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_PreTransformVertices); // <--- Collapses FBX node hierarchies directly into vertices

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << " (" << path << ")" << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    meshes.reserve(scene->mNumMeshes);
    ProcessNode(scene->mRootNode, scene);

    InitMaterials(scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(ProcessMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        ProcessNode(node->mChildren[i], scene);
    }
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // 1. Process Vertices with strict structural bounds safety
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        // Positions
        vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        // Normals
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Texture Coordinates (UVs) - LearnOpenGL explicit channel indexing alignment
        if (mesh->mTextureCoords[0]) {
            // Assimp uses a 3D vector for UVs; map x and y to our vec2 UV coordinate
            vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

            // Tangents (Only parse if UV coordinates exist to avoid stray memory)
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            }
            else {
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            }
        }
        else {
            vertex.uv = glm::vec2(0.0f, 0.0f);
            vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    // 2. Process Indices cleanly per triangular face element
    indices.reserve(static_cast<size_t>(mesh->mNumFaces) * 3);
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // 3. Just record which material slot this mesh uses (OGLDev:
    // MeshEntry.MaterialIndex = paiMesh->mMaterialIndex). Textures are NOT
    // touched here — InitMaterials() loads them once per material, after
    // every mesh has been processed.
    int matIndex = (mesh->mMaterialIndex < scene->mNumMaterials)
        ? static_cast<int>(mesh->mMaterialIndex)
        : Mesh::INVALID_MATERIAL;

    return Mesh(std::move(vertices), std::move(indices), matIndex);
}

void Model::DrawInstanced(
    const Shader& shader,
    const std::vector<glm::mat4>& transforms)
{
    if (transforms.empty())
        return;

    for (const auto& mesh : meshes)
    {
        const Material* material = nullptr;

        if (mesh.materialIndex >= 0 &&
            mesh.materialIndex < static_cast<int>(materials.size()))
        {
            material = &materials[mesh.materialIndex];
        }

        mesh.DrawInstanced(
            shader,
            transforms,
            material
        );
    }
}

void Model::InitMaterials(const aiScene* scene) {
    materials.resize(scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material& material = materials[i];

        // Albedo / Diffuse
        LoadMaterialTexture(mat, aiTextureType_DIFFUSE, "texture_diffuse",
            material.diffuseID, material.hasDiffuse);

        // Normal map
        LoadMaterialTexture(mat, aiTextureType_NORMALS, "texture_normal",
            material.normalID, material.hasNormal);

        // Roughness, with the standard FBX fallback to aiTextureType_UNKNOWN
        LoadMaterialTexture(mat, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness",
            material.roughnessID, material.hasRoughness);
        if (!material.hasRoughness) {
            LoadMaterialTexture(mat, aiTextureType_UNKNOWN, "texture_roughness",
                material.roughnessID, material.hasRoughness);
        }
    }
}

void Model::LoadMaterialTexture(aiMaterial* mat, aiTextureType type, const std::string& typeName,
    unsigned int& outID, bool& outHas) {

    outHas = false;
    outID = 0;

    if (mat->GetTextureCount(type) == 0) return;

    aiString str;
    mat->GetTexture(type, 0, &str);
    outHas = true;

    unsigned int textureID = 0;
    int width = 0;
    int height = 0;
    int nrComponents = 0;
    unsigned char* data = nullptr;

    std::string filename = std::string(str.C_Str());

    // LearnOpenGL strategy: strip away internal folder structures written by Blender
    size_t lastSlash = filename.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filename.substr(lastSlash + 1);
    }

    // Stitch paths: look for textures directly in the same folder as the model
    std::string fullPath = directory + "/" + filename;

    // Fallback path: look inside an explicit /Textures/ sub-folder if missing
    if (!std::filesystem::exists(fullPath)) {
        fullPath = directory + "/Textures/" + filename;
    }

    std::cout << "Assimp/stbi trying to load texture at: " << fullPath << std::endl;

    // Adjust byte packing alignment safety
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1)      format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        GLenum internalFormat = format;
        if (typeName == "texture_diffuse") {
            if (nrComponents == 3)      internalFormat = GL_SRGB;
            else if (nrComponents == 4) internalFormat = GL_SRGB_ALPHA;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "SUCCESSFULLY LOADED TEXTURE ID: " << textureID << std::endl;
    }
    else {
        std::cout << "Texture missing via direct filename. Running keyword search fallback..." << std::endl;
        stbi_image_free(data); // Safely frees null

        // Run a resilient search through the directory for keyword matches
        std::vector<std::string> keywords;
        switch (type) {
        case aiTextureType_DIFFUSE:           keywords = { "color", "albedo", "basecolor", "diffuse" }; break;
        case aiTextureType_NORMALS:           keywords = { "normal", "nrm", "norm" }; break;
        case aiTextureType_DIFFUSE_ROUGHNESS: keywords = { "roughness", "rough" }; break;
        default: break;
        }

        std::vector<std::string> searchDirs = { directory + "/Textures", directory };
        std::string fallbackFoundPath = "";

        for (const auto& dir : searchDirs) {
            if (!fallbackFoundPath.empty() || !std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
                continue;

            for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;

                std::string lowerName = entry.path().filename().string();
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

                bool matches = false;
                for (const auto& kw : keywords) {
                    if (lowerName.find(kw) != std::string::npos) { matches = true; break; }
                }
                if (!matches) continue;

                // Try loading the keyword fallback file
                fallbackFoundPath = entry.path().string();
                data = stbi_load(fallbackFoundPath.c_str(), &width, &height, &nrComponents, 0);
                if (data) break;
            }
        }

        // If the keyword fallback found your actual image (e.g. Rock050_4K-JPG_NormalGL.jpg)
        if (data) {
            std::cout << "Keyword search successfully found: " << fallbackFoundPath << std::endl;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            GLenum format = GL_RGB;
            if (nrComponents == 1)      format = GL_RED;
            else if (nrComponents == 3) format = GL_RGB;
            else if (nrComponents == 4) format = GL_RGBA;

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        else {
            // Absolute Fallback: Generate a clean 1x1 hot-pink placeholder texture,
            // same as OGLDev's white.png fallback so the slot is never left unbound.
            std::cout << "CRITICAL: No fallback found. Creating pink placeholder texture." << std::endl;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            unsigned char pinkPixel[] = { 255, 0, 255 };
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, pinkPixel);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    outID = textureID;
}