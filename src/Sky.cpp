#include "Sky.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

Sky::Sky() {
	skyShader = new Shader(
		"Assets/Shaders/Lighting/Sky/Sky.vert",
		"Assets/Shaders/Lighting/Sky/Sky.frag"
	);

	// Full-Screen triangle 
	float vertices[] =
	{
		-1.0f, -1.0f,
		 3.0f, -1.0f,
		-1.0f,  3.0f
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

	// Ensure sky settings are initialized to non-zero defaults
	settings.skyBrightness = 1.0f;
	settings.horizonHaze = 1.0f;
}

Sky::~Sky()
{
	delete skyShader;

	if (VAO != 0) {
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
	}
}

void Sky::Draw(
	const glm::mat4& view,
	const glm::mat4& projection,
	const glm::vec3& sunDirection
) const {
	if (!skyShader) return;

	// Disable depth writing and set depth function so depth 1.0 passes
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	skyShader->use();

	// Pass unmodified view matrix so ray reconstruction works in shader
	skyShader->setMat4("view", view);
	skyShader->setMat4("projection", projection);

	skyShader->setVec3("sunDirection", glm::normalize(sunDirection));

	// Set sky uniforms
	skyShader->setFloat("skyBrightness", settings.skyBrightness);
	skyShader->setFloat("horizonHaze", settings.horizonHaze);
	skyShader->setFloat("cloudCoverage", settings.cloudCoverage);
	skyShader->setFloat("cloudDensity", settings.cloudDensity);
	skyShader->setFloat("cloudSpeed", settings.cloudSpeed);
	skyShader->setFloat("cloudHeight", settings.cloudHeight);
	skyShader->setInt("cloudsEnabled", settings.cloudsEnabled ? 1 : 0);

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	// Restore default depth states
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}