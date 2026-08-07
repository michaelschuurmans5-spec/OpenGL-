#include "Triangle.h"
#include "stb_image.h"

// Standard C++ 
#include <fstream>
#include <sstream>
#include <iostream>

// Third party
#include <GLFW/glfw3.h>
#include <glad/glad.h> 
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp> // Critical for glm::translate / glm::scale
#include <glm/glm/gtc/type_ptr.hpp>

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
}

// FIXED: Parameter naming fixed to match standard spelling rules + added matrix baking
void Triangle::SpawnCube(const glm::vec3& spawnPosition) {
	std::string name = "Cube_" + std::to_string(nextCubeID);
	nextCubeID++;

	GameObject cube = { ObjectType::Cube, spawnPosition, glm::vec3(1.0f), name, false, glm::vec3(0.0f) };
	cube.transformMatrix = glm::translate(glm::mat4(1.0f), spawnPosition); // FIXED: Passing 2 parameters explicitly
	sceneObjects.push_back(cube);
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

	glUniform3f(glGetUniformLocation(myShader->ID, "objectColor"), 1.0f, 0.5f, 0.31f);
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

	for (const auto& obj : sceneObjects) {
		if (obj.type == ObjectType::Light) continue;

		// Bind the pure, uncorrupted matrix updated natively by ImGuizmo
		glm::mat4 model = obj.transformMatrix;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		if (obj.type == ObjectType::Cube) {
			glUniform1i(useTexLoc, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, cubeTexture);
			glUniform1i(tex1Loc, 0);

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		}
		else if (obj.type == ObjectType::Ground) {
			glUniform1i(useTexLoc, 1);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, groundTexture);
			glUniform1i(tex1Loc, 0);

			glBindVertexArray(groundVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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