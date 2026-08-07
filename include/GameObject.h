#pragma once 

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <string>


enum class ObjectType { Cube, Light, Ground };

struct GameObject {
	ObjectType type;
	glm::vec3 position;
	glm::vec3 scale = glm::vec3(1.0f);
	std::string name;
	bool rotates = true;
	glm::vec3 rotation = glm::vec3(0.0f);

	// ADD THIS: Persistent matrix initialized to Identity
	glm::mat4 transformMatrix = glm::mat4(1.0f);
};

