#pragma once 


#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"


struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};


class Mesh {
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	unsigned int diffuseTexture = 0;  // 0 = no texture, shader falls back to a flat color


	Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds, unsigned int diffuseTex)
		: vertices(std::move(verts)), indices(std::move(inds)), diffuseTexture(diffuseTex) {
		SetupMesh();
	}

	~Mesh() {
		if (VAO != 0) {
			glDeleteVertexArrays(1, &VAO);
			glDeleteBuffers(1, &VBO);
			glDeleteBuffers(1, &EBO);
		}
	}

	// Meshes get moved around when std::vector<Mesh> grows inside Model -
	// without this, the default move would leave two Mesh objects owning
	// (and eventually double-freeing) the same VAO/VBO/EBO.
	Mesh(Mesh&& other) noexcept { *this = std::move(other); }
	Mesh& operator=(Mesh&& other) noexcept {
		if (this != &other) {
			vertices = std::move(other.vertices);
			indices = std::move(other.indices);
			diffuseTexture = other.diffuseTexture;
			VAO = other.VAO; VBO = other.VBO; EBO = other.EBO;
			other.VAO = other.VBO = other.EBO = 0;
		}
		return *this;
	}

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;

	void Draw(const Shader& shader) const {
		if (diffuseTexture != 0) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTexture);
			shader.setInt("diffuseTex", 0);
			shader.setBool("hasTexture", true);
		}
		else {
			shader.setBool("hasTexture", false);
		}

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}

private:
	unsigned int VAO = 0, VBO = 0, EBO = 0;

	void SetupMesh() {
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

		glEnableVertexAttribArray(1); // normal
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

		glEnableVertexAttribArray(2); // uv
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

		glBindVertexArray(0);
	}

};