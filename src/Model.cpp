#include "Model.h"
#include <assimp/include/assimp/Importer.hpp>
#include <assimp/include/assimp/scene.h>
#include <assimp/include/assimp/postprocess.h>
#include <iostream>

#include <stb_image.h>

void Model::LoadModel(const std::string& path) {
	Assimp::Importer importer;
	// Triangulate: force all faces to triangles. FlipUVs: matches OpenGL's
	// bottom-left UV origin. GenSmoothNormals: computes normals if the
	// source file doesn't have them (common on simple rock/prop meshes).
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << " (" << path << ")" << std::endl;
		return;
	}

	directory = path.substr(0, path.find_last_of('/'));
	ProcessNode(scene->mRootNode, scene);
}


void Model::ProcessNode(aiNode* node, const aiScene* scene) {
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(mesh, scene));
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++) {
		ProcessNode(node->mChildren[i], scene);
	}
}

Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene) {
	std::vector<Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);

	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex v{};
		v.position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		v.normal = mesh->HasNormals()
			? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)
			: glm::vec3(0.0f, 1.0f, 0.0f);
		v.uv = mesh->mTextureCoords[0]
			? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
			: glm::vec2(0.0f, 0.0f);
		vertices.push_back(v);
	}

	std::vector<unsigned int> indices;
	indices.reserve((size_t)mesh->mNumFaces * 3);
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	unsigned int diffuseTex = 0;
	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		diffuseTex = LoadMaterialTexture(material, aiTextureType_DIFFUSE, scene);
	}

	return Mesh(std::move(vertices), std::move(indices), diffuseTex);
}

unsigned int Model::LoadMaterialTexture(aiMaterial* mat, int textureType, const aiScene* scene) {
	if (mat->GetTextureCount((aiTextureType)textureType) == 0)
		return 0;

	aiString str;
	mat->GetTexture((aiTextureType)textureType, 0, &str);
	std::string fullPath = directory + "/" + str.C_Str();

	int width, height, channels;
	unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &channels, 0);
	if (!data) {
		std::cout << "ERROR::TEXTURE:: failed to load " << fullPath << std::endl;
		return 0;
	}

	GLenum format = (channels == 1) ? GL_RED : (channels == 3) ? GL_RGB : GL_RGBA;

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	return textureID;
}