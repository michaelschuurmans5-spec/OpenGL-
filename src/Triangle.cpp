#include "Triangle.h"
#include "stb_image.h"

// Standard C++ 
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cmath>

// Third party
#include <GLFW/glfw3.h>
#include <glad/glad.h> 
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp> // Critical for glm::translate / glm::scale
#include <glm/glm/gtc/type_ptr.hpp>

namespace {
	// All generated meshes use the same 8-float stride as the cube
	// (position3, normal3, texcoord2) so they work with the existing shader.
	constexpr GLsizei kMeshStride = 8 * sizeof(float);

	void SetupMeshAttribs() {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, kMeshStride, (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, kMeshStride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, kMeshStride, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	void UploadMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO,
		const std::vector<float>& vertices, const std::vector<unsigned int>& indices) {

		glGenVertexArrays(1, &outVAO);
		glGenBuffers(1, &outVBO);
		glGenBuffers(1, &outEBO);

		glBindVertexArray(outVAO);
		glBindBuffer(GL_ARRAY_BUFFER, outVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, outEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		SetupMeshAttribs();
		glBindVertexArray(0);
	}

	// Unit-radius-0.5 UV sphere, Y-up.
	void CreateSphereMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const unsigned int sectorCount = 24;
		const unsigned int stackCount = 16;
		const float radius = 0.5f;
		const float PI = 3.14159265359f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		for (unsigned int i = 0; i <= stackCount; ++i) {
			float phi = PI / 2.0f - i * (PI / stackCount); // +90 down to -90
			float y = radius * sinf(phi);
			float r = radius * cosf(phi);

			for (unsigned int j = 0; j <= sectorCount; ++j) {
				float theta = j * (2.0f * PI / sectorCount);
				float x = r * cosf(theta);
				float z = r * sinf(theta);

				vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
				glm::vec3 n = glm::normalize(glm::vec3(x, y, z));
				vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
				vertices.push_back((float)j / sectorCount); vertices.push_back((float)i / stackCount);
			}
		}

		for (unsigned int i = 0; i < stackCount; ++i) {
			unsigned int k1 = i * (sectorCount + 1);
			unsigned int k2 = k1 + sectorCount + 1;

			for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
				if (i != 0) {
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}
				if (i != stackCount - 1) {
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Radius-0.5, height-1 cylinder, Y-up, with smooth side normals and flat cap normals.
	void CreateCylinderMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const unsigned int sectorCount = 24;
		const float radius = 0.5f;
		const float halfHeight = 0.5f;
		const float PI = 3.14159265359f;

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		// Side wall (two rings, radial normals)
		for (unsigned int ring = 0; ring < 2; ++ring) {
			float y = (ring == 0) ? -halfHeight : halfHeight;
			for (unsigned int j = 0; j <= sectorCount; ++j) {
				float theta = j * (2.0f * PI / sectorCount);
				float x = radius * cosf(theta);
				float z = radius * sinf(theta);

				vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
				glm::vec3 n = glm::normalize(glm::vec3(x, 0.0f, z));
				vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z);
				vertices.push_back((float)j / sectorCount); vertices.push_back((float)ring);
			}
		}
		unsigned int ringVerts = sectorCount + 1;
		for (unsigned int j = 0; j < sectorCount; ++j) {
			unsigned int b0 = j, b1 = j + 1;
			unsigned int t0 = ringVerts + j, t1 = ringVerts + j + 1;
			indices.push_back(b0); indices.push_back(t0); indices.push_back(b1);
			indices.push_back(b1); indices.push_back(t0); indices.push_back(t1);
		}

		// Bottom cap
		unsigned int bottomCenter = (unsigned int)(vertices.size() / 8);
		vertices.push_back(0.0f); vertices.push_back(-halfHeight); vertices.push_back(0.0f);
		vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
		vertices.push_back(0.5f); vertices.push_back(0.5f);

		unsigned int bottomRim = (unsigned int)(vertices.size() / 8);
		for (unsigned int j = 0; j <= sectorCount; ++j) {
			float theta = j * (2.0f * PI / sectorCount);
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);
			vertices.push_back(x); vertices.push_back(-halfHeight); vertices.push_back(z);
			vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
			vertices.push_back(0.5f + 0.5f * cosf(theta)); vertices.push_back(0.5f + 0.5f * sinf(theta));
		}
		for (unsigned int j = 0; j < sectorCount; ++j) {
			indices.push_back(bottomCenter);
			indices.push_back(bottomRim + j + 1);
			indices.push_back(bottomRim + j);
		}

		// Top cap
		unsigned int topCenter = (unsigned int)(vertices.size() / 8);
		vertices.push_back(0.0f); vertices.push_back(halfHeight); vertices.push_back(0.0f);
		vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
		vertices.push_back(0.5f); vertices.push_back(0.5f);

		unsigned int topRim = (unsigned int)(vertices.size() / 8);
		for (unsigned int j = 0; j <= sectorCount; ++j) {
			float theta = j * (2.0f * PI / sectorCount);
			float x = radius * cosf(theta);
			float z = radius * sinf(theta);
			vertices.push_back(x); vertices.push_back(halfHeight); vertices.push_back(z);
			vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
			vertices.push_back(0.5f + 0.5f * cosf(theta)); vertices.push_back(0.5f + 0.5f * sinf(theta));
		}
		for (unsigned int j = 0; j < sectorCount; ++j) {
			indices.push_back(topCenter);
			indices.push_back(topRim + j);
			indices.push_back(topRim + j + 1);
		}

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Unit quad on the XZ plane, facing +Y - a placeable "Plane" primitive,
	// distinct from the large static Ground mesh already in the scene.
	void CreatePlaneMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO) {
		std::vector<float> vertices = {
			-0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
			 0.5f, 0.0f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
			 0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
			-0.5f, 0.0f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f
		};
		std::vector<unsigned int> indices = { 0, 1, 2, 0, 2, 3 };

		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}

	// Triangular prism (equilateral cross-section), Y-up, flat-shaded faces.
	void CreatePrismMesh(unsigned int& outVAO, unsigned int& outVBO, unsigned int& outEBO, unsigned int& outIndexCount) {
		const float halfHeight = 0.5f;
		glm::vec2 p0(0.0f, 0.5f);
		glm::vec2 p1(-0.433f, -0.25f);
		glm::vec2 p2(0.433f, -0.25f);

		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		auto pushVert = [&](const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& uv) {
			vertices.push_back(pos.x); vertices.push_back(pos.y); vertices.push_back(pos.z);
			vertices.push_back(normal.x); vertices.push_back(normal.y); vertices.push_back(normal.z);
			vertices.push_back(uv.x); vertices.push_back(uv.y);
			};

		auto addTri = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
			glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
			unsigned int start = (unsigned int)(vertices.size() / 8);
			pushVert(a, n, glm::vec2(0.0f, 0.0f));
			pushVert(b, n, glm::vec2(1.0f, 0.0f));
			pushVert(c, n, glm::vec2(0.5f, 1.0f));
			indices.push_back(start); indices.push_back(start + 1); indices.push_back(start + 2);
			};

		auto addQuad = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& d) {
			glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
			unsigned int start = (unsigned int)(vertices.size() / 8);
			pushVert(a, n, glm::vec2(0.0f, 0.0f));
			pushVert(b, n, glm::vec2(1.0f, 0.0f));
			pushVert(c, n, glm::vec2(1.0f, 1.0f));
			pushVert(d, n, glm::vec2(0.0f, 1.0f));
			indices.push_back(start); indices.push_back(start + 1); indices.push_back(start + 2);
			indices.push_back(start); indices.push_back(start + 2); indices.push_back(start + 3);
			};

		glm::vec3 b0(p0.x, -halfHeight, p0.y), b1(p1.x, -halfHeight, p1.y), b2(p2.x, -halfHeight, p2.y);
		glm::vec3 t0(p0.x, halfHeight, p0.y), t1(p1.x, halfHeight, p1.y), t2(p2.x, halfHeight, p2.y);

		addTri(b0, b2, b1);   // bottom cap
		addTri(t0, t1, t2);   // top cap
		addQuad(b0, b1, t1, t0);
		addQuad(b1, b2, t2, t1);
		addQuad(b2, b0, t0, t2);

		outIndexCount = (unsigned int)indices.size();
		UploadMesh(outVAO, outVBO, outEBO, vertices, indices);
	}
}

Triangle::Triangle() {

	// Shader loading and compiling 
	myShader = new Shader("Assets/Shaders/Shapes/Triangle.vert", "Assets/Shaders/Shapes/Triangle.frag");
	lightCubeShader = new Shader("Assets/Shaders/Shapes/LightCube.vert", "Assets/Shaders/Shapes/LightCube.frag");

	// SET UP DATA (Positions, Normals, TexCoords)
	float vertices[] = {
		// Positions          // Normals           // TexCoords
		// Back Face
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,

		// Front Face
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,

		// Left Face
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		// Right Face
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

		 // Bottom Face
		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
		  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
		 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

		 // Top Face
		 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
		  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
		  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
		 -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
	};

	unsigned int indices[] = {
		0,  2,  1,  0,  3,  2,  // Back
		4,  5,  6,  4,  6,  7,  // Front
		8,  9,  10, 8,  10, 11, // Left
		12, 14, 13, 12, 15, 14, // Right
		16, 17, 18, 16, 18, 19, // Bottom
		20, 22, 21, 20, 23, 22  // Top
	};

	// Allocate containers inside VRAM
	glGenVertexArrays(1, &VAO);
	glGenVertexArrays(1, &lightVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// REPLACED OLD LINES HERE: Initialize GameObjects using the new structural alignment
	GameObject cube = { ObjectType::Cube, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f), "Cube_0", false, glm::vec3(0.0f) };
	cube.transformMatrix = glm::translate(glm::mat4(1.0f), cube.position);
	cube.textureSlot = TextureSlot::Container;
	sceneObjects.push_back(cube);

	GameObject ground = { ObjectType::Ground, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f), "Ground", false, glm::vec3(0.0f) };
	ground.transformMatrix = glm::translate(glm::mat4(1.0f), ground.position);
	sceneObjects.push_back(ground);

	// Stride for the cube: 8 floats
	GLsizei stride = 8 * sizeof(float);

	// A. CONFIGURE MAIN OBJECT VAO
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Positions attribute Location 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	// Normal attribute Location 1 
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// TexCoords attribute location 
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// B. CONFIGURE LIGHT SOURCE VAO (Using same VBO/EBO)
	glBindVertexArray(lightVAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

	// The light source cube only needs the positions vector attribute layout (Location 0)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	// C. CONFIGURE GROUND PLANE VAO
	float groundVertices[] = {
		// positions            // normals        // TexCoords
		-5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 0.0f,
		 5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   5.0f, 5.0f,
		-5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,   0.0f, 5.0f
	};
	unsigned int groundIndices[] = { 0, 1, 2, 0, 2, 3 };

	GLsizei groundStride = 8 * sizeof(float);

	glGenVertexArrays(1, &groundVAO);
	glGenBuffers(1, &groundVBO);
	glGenBuffers(1, &groundEBO);

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);

	// GROUND TEXTURE - Grass
	glGenTextures(1, &groundTexture);
	glBindTexture(GL_TEXTURE_2D, groundTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	unsigned char* data = stbi_load("Assets/Textures/Shapes/ShapeTxt/BasicGrass.jpg", &width, &height, &nrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "ERROR: FAILED TO LOAD GRASS TEXTURE FROM DISK!" << std::endl;
	}
	stbi_image_free(data);

	// CUBE TEXTURE - Container
	glGenTextures(1, &cubeTexture);
	glBindTexture(GL_TEXTURE_2D, cubeTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load("Assets/Textures/Shapes/ShapeTxt/container.jpg", &width, &height, &nrChannels, 0);
	if (data) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else {
		std::cerr << "ERROR: FAILED TO LOAD CUBE CONTAINER TEXTURE FROM DISK!" << std::endl;
	}
	stbi_image_free(data);

	// Configure Ground Buffer Data structures
	glBindVertexArray(groundVAO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(groundVertices), groundVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(groundIndices), groundIndices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, groundStride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, groundStride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, groundStride, (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	// Unbind everything safely
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// D. GENERATE BASIC-SHAPE MESHES (Sphere, Cylinder, Plane, Prism)
	// used by the Objects > Shapes drag-and-drop panel.
	CreateSphereMesh(sphereVAO, sphereVBO, sphereEBO, sphereIndexCount);
	CreateCylinderMesh(cylinderVAO, cylinderVBO, cylinderEBO, cylinderIndexCount);
	CreatePlaneMesh(planeVAO, planeVBO, planeEBO);
	CreatePrismMesh(prismVAO, prismVBO, prismEBO, prismIndexCount);

	glBindVertexArray(0);
}

// FIXED: Parameter naming fixed to match standard spelling rules + added matrix baking
void Triangle::SpawnCube(const glm::vec3& spawnPosition) {
	std::string name = "Cube_" + std::to_string(nextCubeID);
	nextCubeID++;

	GameObject cube = { ObjectType::Cube, spawnPosition, glm::vec3(1.0f), name, false, glm::vec3(0.0f) };
	cube.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition); // FIXED: Passing 2 parameters explicitly
	cube.textureSlot = TextureSlot::Container;
	sceneObjects.push_back(cube);
}

// Generic spawn used by the Objects > Shapes properties panel's "Create"
// button and by dropping a shape from that menu into the viewport.
// Each ObjectType is drawn with its own dedicated mesh (see Draw()).
void Triangle::SpawnShape(ObjectType type, const glm::vec3& spawnPosition, const std::string& customName,
	const glm::vec3& baseColor, TextureSlot textureSlot) {

	std::string name = customName.empty()
		? (ToString(type) + "_" + std::to_string(nextCubeID))
		: customName;
	nextCubeID++;

	GameObject obj{};
	obj.type = type;
	obj.position = spawnPosition;
	obj.scale = glm::vec3(1.0f);
	obj.name = name;
	obj.rotates = false;
	obj.rotation = glm::vec3(0.0f);
	obj.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition);
	obj.baseColor = baseColor;
	obj.textureSlot = textureSlot;
	obj.isBasicShape = true;

	sceneObjects.push_back(obj);
}

// FIXED: Added matrix baking configuration to light entities
void Triangle::SpawnLight(const glm::vec3& spawnPosition) {
	std::string name = "Light_" + std::to_string(nextLightID);
	nextLightID++;

	GameObject light = { ObjectType::Light, spawnPosition, glm::vec3(1.0f), name, false, glm::vec3(0.0f) };
	light.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition); // FIXED: Passing 2 parameters explicitly
	sceneObjects.push_back(light);
}

Triangle::~Triangle() {
	delete myShader;
	delete lightCubeShader;

	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glDeleteVertexArrays(1, &groundVAO);
	glDeleteBuffers(1, &groundVBO);
	glDeleteBuffers(1, &groundEBO);
	glDeleteTextures(1, &groundTexture);
	glDeleteTextures(1, &cubeTexture);

	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);

	glDeleteVertexArrays(1, &cylinderVAO);
	glDeleteBuffers(1, &cylinderVBO);
	glDeleteBuffers(1, &cylinderEBO);

	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);
	glDeleteBuffers(1, &planeEBO);

	glDeleteVertexArrays(1, &prismVAO);
	glDeleteBuffers(1, &prismVBO);
	glDeleteBuffers(1, &prismEBO);
}

void Triangle::Draw(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::vec3& lightPos, const glm::vec3& cameraPos) const {

	const int MAX_LIGHTS = 8;
	std::vector<glm::vec3> activeLightPositions;

	for (const auto& obj : sceneObjects) {
		if (obj.type == ObjectType::Light) {
			activeLightPositions.push_back(obj.position);
			if (activeLightPositions.size() >= MAX_LIGHTS) break;
		}
	}

	bool hasLightEntity = !activeLightPositions.empty();

	if (!hasLightEntity) {
		activeLightPositions.push_back(lightPos);
	}

	// ----------------------------------------------------
	// PASS 1: RENDER MAIN SCENE ENTITIES (CUBE & GROUND)
	// ----------------------------------------------------
	myShader->use();

	glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(myShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

	glUniform3f(glGetUniformLocation(myShader->ID, "lightColor"), 1.0f, 1.0f, 1.0f);

	glUniform3fv(
		glGetUniformLocation(myShader->ID, "lightPositions"),
		(GLsizei)activeLightPositions.size(),
		glm::value_ptr(activeLightPositions[0]));

	glUniform1i(
		glGetUniformLocation(myShader->ID, "numLights"),
		(GLsizei)activeLightPositions.size()
	);

	glUniform3fv(glGetUniformLocation(myShader->ID, "viewPos"), 1, glm::value_ptr(cameraPos));

	int modelLoc = glGetUniformLocation(myShader->ID, "model");
	int useTexLoc = glGetUniformLocation(myShader->ID, "useTexture");
	int tex1Loc = glGetUniformLocation(myShader->ID, "texture1");
	int objectColorLoc = glGetUniformLocation(myShader->ID, "objectColor");

	for (const auto& obj : sceneObjects) {
		if (obj.type == ObjectType::Light) continue;

		// Bind the pure, uncorrupted matrix updated natively by ImGuizmo
		glm::mat4 model = obj.transformMatrix;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		if (obj.type == ObjectType::Ground) {
			glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);
			glUniform1i(useTexLoc, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, groundTexture);
			glUniform1i(tex1Loc, 0);

			glBindVertexArray(groundVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		else {
			// Basic shapes (Cube, Sphere, Plane, Cylinder, Prism) now each
			// render with their own generated mesh; color/texture stay per-object.
			glUniform3f(objectColorLoc, obj.baseColor.r, obj.baseColor.g, obj.baseColor.b);

			GLuint texToBind = 0;
			bool useTex = false;
			if (obj.textureSlot == TextureSlot::Container) { texToBind = cubeTexture; useTex = true; }
			else if (obj.textureSlot == TextureSlot::Grass) { texToBind = groundTexture; useTex = true; }

			glUniform1i(useTexLoc, useTex ? 1 : 0);
			if (useTex) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texToBind);
				glUniform1i(tex1Loc, 0);
			}

			switch (obj.type) {
			case ObjectType::Sphere:
				glBindVertexArray(sphereVAO);
				glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Cylinder:
				glBindVertexArray(cylinderVAO);
				glDrawElements(GL_TRIANGLES, cylinderIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Plane:
				glBindVertexArray(planeVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Prism:
				glBindVertexArray(prismVAO);
				glDrawElements(GL_TRIANGLES, prismIndexCount, GL_UNSIGNED_INT, 0);
				break;
			case ObjectType::Cube:
			default:
				glBindVertexArray(VAO);
				glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
				break;
			}
		}
	}

	// ----------------------------------------------------
	// PASS 2: RENDER THE PHYSICAL LIGHT SOURCE CUBE
	// ----------------------------------------------------
	lightCubeShader->use();

	glUniformMatrix4fv(glGetUniformLocation(lightCubeShader->ID, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
	glUniformMatrix4fv(glGetUniformLocation(lightCubeShader->ID, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
	int lightModelLoc = glGetUniformLocation(lightCubeShader->ID, "model");

	if (hasLightEntity) {
		for (const auto& obj : sceneObjects) {
			if (obj.type != ObjectType::Light) continue;

			// Extract the light matrix managed dynamically inside your viewport space
			glm::mat4 lightModel = obj.transformMatrix;
			lightModel = glm::scale(lightModel, glm::vec3(0.2f));

			glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
			glBindVertexArray(lightVAO);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
	}
	else {
		glm::mat4 lightModel = glm::mat4(1.0f);
		lightModel = glm::translate(lightModel, lightPos);
		lightModel = glm::scale(lightModel, glm::vec3(0.2f));
		glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
		glBindVertexArray(lightVAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}