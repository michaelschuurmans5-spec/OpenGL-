#pragma once 


#include <string>
#include <vector>
#include "Mesh.h"



class Model {
public:
	Model() = default;
	explicit Model(const std::string& path) { LoadModel(path); }

	void Draw(const Shader& shader) const {
		for (const auto& mesh : meshes)
			mesh.Draw(shader);
	}

	bool IsLoaded() const { return !meshes.empty(); }

private:
	std::vector<Mesh> meshes;
	std::string directory;

	void LoadModel(const std::string& path);
	void ProcessNode(struct aiNode* node, const struct aiScene* scene);
	Mesh ProcessMesh(struct aiMesh* mesh, const struct aiScene* scene);
	unsigned int LoadMaterialTexture(struct aiMaterial* mat, int textureType, const struct aiScene* scene);
};