#include "sky.h"


#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

Sky::Sky() {
	skyShader = new Shader(
		"Assets/Shaders/lighting/Sky/Sky.vert",
		"Assets/Shaders/Lighting/Sky/Sky.frag"
	);


	// Full-Screen triangle 

	float vertices[] =
	{
		-1.0f, -1.0f,
		3.0f, -1.0f,
		-1.0f, 3.0f
	};


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(
		GL_ARRAY_BUFFER,
		sizeof(vertices),
		vertices,
		GL_STATIC_DRAW
	);

	glEnableVertexAttribArray(0);

	glVertexAttribPointer(
		0,
		2,
		GL_FLOAT,
		GL_FALSE,
		2 * sizeof(float),
		(void*)0
	);

	glBindVertexArray(0);
}

Sky::~Sky()
{
	delete skyShader;

	if (VAO != 0)
		glDeleteBuffers(1, &VBO);
}

void Sky::Draw(
	const glm::mat4& view,
	const glm::mat4& projection,
	const glm::vec3& sunDirection
)const
{
	if (!skyShader)
		return;

	skyShader->use();

	// Remive Camera translatioin

	// The sky should follow the camera position 
	glm::mat4 skyView = glm::mat4(glm::mat3(view));

	skyShader->setMat4("view", skyView);
	skyShader->setMat4("projection", projection);

	skyShader->setVec3("subDirection", sunDirection);

	skyShader->setFloat(
		"skyBrightness",
		settings.skyBrightness
	);

	skyShader->setFloat(
		"horizonHaze",
		settings.horizonHaze
	);

	skyShader->setFloat(
		"cloudCoverage",
		settings.cloudCoverage
	);

	skyShader->setFloat(
		"cloudDensity",
		settings.cloudDensity
	);

	skyShader->setFloat(
		"cloudSpeed",
		settings.cloudSpeed
	);

	skyShader->setFloat(
		"cloudHeight",
		settings.cloudHeight
	);

	skyShader->setInt(
		"cloudsEnabled",
		settings.cloudsEnabled ? 1 : 0
	);

	glBindVertexArray(VAO);

	glDrawArrays(
		GL_TRIANGLES,
		0,
		3
	);

	glBindVertexArray(0);
}